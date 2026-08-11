#include "render/gpu_mesh.hpp"

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>

namespace hover::render {
namespace {

struct GpuBufferDeleter {
    SDL_GPUDevice* device;

    void operator()(SDL_GPUBuffer* buffer) const noexcept { SDL_ReleaseGPUBuffer(device, buffer); }
};

using ScopedGpuBuffer = std::unique_ptr<SDL_GPUBuffer, GpuBufferDeleter>;

struct TransferBufferDeleter {
    SDL_GPUDevice* device;

    void operator()(SDL_GPUTransferBuffer* buffer) const noexcept {
        SDL_ReleaseGPUTransferBuffer(device, buffer);
    }
};

using ScopedTransferBuffer = std::unique_ptr<SDL_GPUTransferBuffer, TransferBufferDeleter>;

} // namespace

GpuMesh::GpuMesh(SDL_GPUDevice* device) : device_(device) {}

GpuMesh::~GpuMesh() {
    if (index_buffer_ != nullptr) {
        SDL_ReleaseGPUBuffer(device_, index_buffer_);
    }
    if (vertex_buffer_ != nullptr) {
        SDL_ReleaseGPUBuffer(device_, vertex_buffer_);
    }
}

bool GpuMesh::upload(const MeshData& mesh, std::string_view label) {
    constexpr std::size_t maximum_upload_size = std::numeric_limits<Uint32>::max();
    const std::string_view display_label = label.empty() ? std::string_view{"unnamed"} : label;
    if (!is_valid(mesh)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot upload invalid mesh '%.*s'.",
                     static_cast<int>(display_label.size()), display_label.data());
        return false;
    }
    if (mesh.vertices.size() > maximum_upload_size / sizeof(Vertex) ||
        mesh.indices.size() > maximum_upload_size / sizeof(std::uint32_t)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mesh '%.*s' is too large for SDL_GPU upload.",
                     static_cast<int>(display_label.size()), display_label.data());
        return false;
    }

    const std::size_t vertex_byte_count = mesh.vertices.size() * sizeof(Vertex);
    const std::size_t index_byte_count = mesh.indices.size() * sizeof(std::uint32_t);
    if (vertex_byte_count > maximum_upload_size - index_byte_count) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Combined buffers for mesh '%.*s' are too large for one SDL_GPU upload.",
                     static_cast<int>(display_label.size()), display_label.data());
        return false;
    }
    const Uint32 vertex_bytes = static_cast<Uint32>(vertex_byte_count);
    const Uint32 index_bytes = static_cast<Uint32>(index_byte_count);

    SDL_GPUBufferCreateInfo vertex_info{};
    vertex_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_info.size = vertex_bytes;
    ScopedGpuBuffer vertex_buffer{SDL_CreateGPUBuffer(device_, &vertex_info),
                                  GpuBufferDeleter{device_}};
    if (!vertex_buffer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create vertex buffer for '%.*s': %s",
                     static_cast<int>(display_label.size()), display_label.data(), SDL_GetError());
        return false;
    }

    SDL_GPUBufferCreateInfo index_info{};
    index_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_info.size = index_bytes;
    ScopedGpuBuffer index_buffer{SDL_CreateGPUBuffer(device_, &index_info),
                                 GpuBufferDeleter{device_}};
    if (!index_buffer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create index buffer for '%.*s': %s",
                     static_cast<int>(display_label.size()), display_label.data(), SDL_GetError());
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transfer_info{};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = vertex_bytes + index_bytes;
    ScopedTransferBuffer transfer_buffer{SDL_CreateGPUTransferBuffer(device_, &transfer_info),
                                         TransferBufferDeleter{device_}};
    if (!transfer_buffer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Could not create transfer buffer for '%.*s': %s",
                     static_cast<int>(display_label.size()), display_label.data(), SDL_GetError());
        return false;
    }

    void* mapped_data = SDL_MapGPUTransferBuffer(device_, transfer_buffer.get(), false);
    if (mapped_data == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not map transfer buffer for '%.*s': %s",
                     static_cast<int>(display_label.size()), display_label.data(), SDL_GetError());
        return false;
    }
    std::memcpy(mapped_data, mesh.vertices.data(), vertex_bytes);
    std::memcpy(static_cast<std::byte*>(mapped_data) + vertex_bytes, mesh.indices.data(),
                index_bytes);
    SDL_UnmapGPUTransferBuffer(device_, transfer_buffer.get());

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device_);
    if (command_buffer == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Could not acquire upload command buffer for '%.*s': %s",
                     static_cast<int>(display_label.size()), display_label.data(), SDL_GetError());
        return false;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (copy_pass == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not begin copy pass for '%.*s': %s",
                     static_cast<int>(display_label.size()), display_label.data(), SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer);
        return false;
    }

    const SDL_GPUTransferBufferLocation vertex_source{transfer_buffer.get(), 0};
    const SDL_GPUBufferRegion vertex_destination{vertex_buffer.get(), 0, vertex_bytes};
    SDL_UploadToGPUBuffer(copy_pass, &vertex_source, &vertex_destination, false);

    const SDL_GPUTransferBufferLocation index_source{transfer_buffer.get(), vertex_bytes};
    const SDL_GPUBufferRegion index_destination{index_buffer.get(), 0, index_bytes};
    SDL_UploadToGPUBuffer(copy_pass, &index_source, &index_destination, false);
    SDL_EndGPUCopyPass(copy_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not submit upload for '%.*s': %s",
                     static_cast<int>(display_label.size()), display_label.data(), SDL_GetError());
        return false;
    }

    if (index_buffer_ != nullptr) {
        SDL_ReleaseGPUBuffer(device_, index_buffer_);
    }
    if (vertex_buffer_ != nullptr) {
        SDL_ReleaseGPUBuffer(device_, vertex_buffer_);
    }
    vertex_buffer_ = vertex_buffer.release();
    index_buffer_ = index_buffer.release();
    index_count_ = static_cast<Uint32>(mesh.indices.size());

    SDL_Log("Uploaded mesh '%.*s': %zu vertices and %zu indices.",
            static_cast<int>(display_label.size()), display_label.data(), mesh.vertices.size(),
            mesh.indices.size());
    return true;
}

} // namespace hover::render
