#pragma once

#include "render/mesh_data.hpp"

#include <SDL3/SDL_gpu.h>

#include <string_view>

namespace hover::render {

class GpuMesh final {
  public:
    explicit GpuMesh(SDL_GPUDevice* device);
    ~GpuMesh();

    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
    GpuMesh(GpuMesh&&) = delete;
    GpuMesh& operator=(GpuMesh&&) = delete;

    [[nodiscard]] bool upload(const MeshData& mesh, std::string_view label);

    [[nodiscard]] SDL_GPUBuffer* vertex_buffer() const { return vertex_buffer_; }
    [[nodiscard]] SDL_GPUBuffer* index_buffer() const { return index_buffer_; }
    [[nodiscard]] Uint32 index_count() const { return index_count_; }

  private:
    SDL_GPUDevice* device_;
    SDL_GPUBuffer* vertex_buffer_ = nullptr;
    SDL_GPUBuffer* index_buffer_ = nullptr;
    Uint32 index_count_ = 0;
};

} // namespace hover::render
