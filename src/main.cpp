#include <SDL3/SDL.h>

#include "assets/generated/presentation_pad.hpp"
#include "assets/generated/prototype_01_mesh.hpp"
#include "game/ships/prototype_01.hpp"
#include "hover_math.hpp"
#include "render/gpu_mesh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>

namespace {

constexpr int initial_window_width = 1280;
constexpr int initial_window_height = 720;

#if defined(NDEBUG)
constexpr bool gpu_debug_mode = false;
#else
constexpr bool gpu_debug_mode = true;
#endif

struct WindowDeleter {
    void operator()(SDL_Window* window) const noexcept { SDL_DestroyWindow(window); }
};

using Window = std::unique_ptr<SDL_Window, WindowDeleter>;

struct GpuDeviceDeleter {
    void operator()(SDL_GPUDevice* device) const noexcept { SDL_DestroyGPUDevice(device); }
};

using GpuDevice = std::unique_ptr<SDL_GPUDevice, GpuDeviceDeleter>;

struct GpuTextureDeleter {
    SDL_GPUDevice* device;

    void operator()(SDL_GPUTexture* texture) const noexcept {
        SDL_ReleaseGPUTexture(device, texture);
    }
};

using GpuTexture = std::unique_ptr<SDL_GPUTexture, GpuTextureDeleter>;

struct SdlMemoryDeleter {
    void operator()(void* memory) const noexcept { SDL_free(memory); }
};

using SdlMemory = std::unique_ptr<void, SdlMemoryDeleter>;

struct GpuShaderDeleter {
    SDL_GPUDevice* device;

    void operator()(SDL_GPUShader* shader) const noexcept { SDL_ReleaseGPUShader(device, shader); }
};

using GpuShader = std::unique_ptr<SDL_GPUShader, GpuShaderDeleter>;

struct GraphicsPipelineDeleter {
    SDL_GPUDevice* device;

    void operator()(SDL_GPUGraphicsPipeline* pipeline) const noexcept {
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    }
};

using GraphicsPipeline = std::unique_ptr<SDL_GPUGraphicsPipeline, GraphicsPipelineDeleter>;

class GpuWindowClaim final {
  public:
    GpuWindowClaim(SDL_GPUDevice* device, SDL_Window* window) : device_(device), window_(window) {}

    GpuWindowClaim(const GpuWindowClaim&) = delete;
    GpuWindowClaim& operator=(const GpuWindowClaim&) = delete;

    ~GpuWindowClaim() { SDL_ReleaseWindowFromGPUDevice(device_, window_); }

  private:
    SDL_GPUDevice* device_;
    SDL_Window* window_;
};

class DepthTarget final {
  public:
    explicit DepthTarget(SDL_GPUDevice* device)
        : device_(device), texture_(nullptr, GpuTextureDeleter{device}) {}

    DepthTarget(const DepthTarget&) = delete;
    DepthTarget& operator=(const DepthTarget&) = delete;

    [[nodiscard]] bool ensure_size(Uint32 width, Uint32 height) {
        if (texture_ && width == width_ && height == height_) {
            return true;
        }

        SDL_GPUTextureCreateInfo texture_info{};
        texture_info.type = SDL_GPU_TEXTURETYPE_2D;
        texture_info.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
        texture_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        texture_info.width = width;
        texture_info.height = height;
        texture_info.layer_count_or_depth = 1;
        texture_info.num_levels = 1;
        texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;

        GpuTexture replacement{SDL_CreateGPUTexture(device_, &texture_info),
                               GpuTextureDeleter{device_}};
        if (!replacement) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Could not create the %ux%u depth target: %s", width, height,
                         SDL_GetError());
            return false;
        }

        texture_ = std::move(replacement);
        width_ = width;
        height_ = height;
        return true;
    }

    [[nodiscard]] SDL_GPUTexture* texture() const { return texture_.get(); }

  private:
    SDL_GPUDevice* device_;
    GpuTexture texture_;
    Uint32 width_ = 0;
    Uint32 height_ = 0;
};

bool requests_exit(const SDL_Event& event) {
    return event.type == SDL_EVENT_QUIT ||
           (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE);
}

enum class RenderResult : std::uint8_t {
    presented,
    skipped,
    failed,
};

class FrameStatistics final {
  public:
    explicit FrameStatistics(SDL_Window* window)
        : window_(window),
          performance_frequency_(static_cast<double>(SDL_GetPerformanceFrequency())) {}

    void record_presented_frame() {
        const Uint64 current_counter = SDL_GetPerformanceCounter();
        if (previous_counter_ == 0) {
            previous_counter_ = current_counter;
            return;
        }

        const double frame_seconds =
            static_cast<double>(current_counter - previous_counter_) / performance_frequency_;
        previous_counter_ = current_counter;

        sample_elapsed_seconds_ += frame_seconds;
        sample_frame_seconds_ += frame_seconds;
        if (frame_seconds > sample_worst_frame_seconds_) {
            sample_worst_frame_seconds_ = frame_seconds;
        }
        ++sample_frame_count_;

        if (sample_elapsed_seconds_ < 1.0) {
            return;
        }

        const double frames_per_second =
            static_cast<double>(sample_frame_count_) / sample_elapsed_seconds_;
        const double average_frame_ms =
            (sample_frame_seconds_ / static_cast<double>(sample_frame_count_)) * 1000.0;
        const double worst_frame_ms = sample_worst_frame_seconds_ * 1000.0;

        SDL_Log("Frame timing: %.1f FPS | %.3f ms average | %.3f ms worst", frames_per_second,
                average_frame_ms, worst_frame_ms);

        if (title_updates_enabled_) {
            char title[128]{};
            SDL_snprintf(title, sizeof(title),
                         "Codename Hover | %.1f FPS | %.2f ms average | %.2f ms worst",
                         frames_per_second, average_frame_ms, worst_frame_ms);
            if (!SDL_SetWindowTitle(window_, title)) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not update the window title: %s",
                            SDL_GetError());
                title_updates_enabled_ = false;
            }
        }

        reset_sample();
    }

    void reset_after_skipped_frame() {
        previous_counter_ = 0;
        reset_sample();
    }

  private:
    void reset_sample() {
        sample_elapsed_seconds_ = 0.0;
        sample_frame_seconds_ = 0.0;
        sample_worst_frame_seconds_ = 0.0;
        sample_frame_count_ = 0;
    }

    SDL_Window* window_;
    double performance_frequency_;
    Uint64 previous_counter_ = 0;
    double sample_elapsed_seconds_ = 0.0;
    double sample_frame_seconds_ = 0.0;
    double sample_worst_frame_seconds_ = 0.0;
    size_t sample_frame_count_ = 0;
    bool title_updates_enabled_ = true;
};

GpuShader load_shader(SDL_GPUDevice* device, const std::string& path, SDL_GPUShaderStage stage) {
    size_t code_size = 0;
    SdlMemory code{SDL_LoadFile(path.c_str(), &code_size)};
    if (!code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not load shader '%s': %s", path.c_str(),
                     SDL_GetError());
        return GpuShader{nullptr, GpuShaderDeleter{device}};
    }

    SDL_GPUShaderCreateInfo shader_info{};
    shader_info.code_size = code_size;
    shader_info.code = static_cast<const Uint8*>(code.get());
    shader_info.entrypoint = "main";
    shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    shader_info.stage = stage;
    shader_info.num_uniform_buffers = stage == SDL_GPU_SHADERSTAGE_VERTEX ? 1U : 0U;

    GpuShader shader{SDL_CreateGPUShader(device, &shader_info), GpuShaderDeleter{device}};
    if (!shader) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create shader '%s': %s", path.c_str(),
                     SDL_GetError());
    }

    return shader;
}

GraphicsPipeline create_vehicle_pipeline(SDL_GPUDevice* device, SDL_Window* window) {
    const char* base_path = SDL_GetBasePath();
    if (base_path == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not find the executable directory: %s",
                     SDL_GetError());
        return GraphicsPipeline{nullptr, GraphicsPipelineDeleter{device}};
    }

    const std::string shader_directory = std::string{base_path} + "shaders/";
    GpuShader vertex_shader =
        load_shader(device, shader_directory + "vehicle_vertex.spv", SDL_GPU_SHADERSTAGE_VERTEX);
    if (!vertex_shader) {
        return GraphicsPipeline{nullptr, GraphicsPipelineDeleter{device}};
    }

    GpuShader fragment_shader = load_shader(device, shader_directory + "vehicle_fragment.spv",
                                            SDL_GPU_SHADERSTAGE_FRAGMENT);
    if (!fragment_shader) {
        return GraphicsPipeline{nullptr, GraphicsPipelineDeleter{device}};
    }

    SDL_GPUColorTargetDescription color_target{};
    color_target.format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUVertexBufferDescription vertex_buffer_description{};
    vertex_buffer_description.slot = 0;
    vertex_buffer_description.pitch = static_cast<Uint32>(sizeof(hover::render::Vertex));
    vertex_buffer_description.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    const std::array vertex_attributes{
        SDL_GPUVertexAttribute{0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                               static_cast<Uint32>(offsetof(hover::render::Vertex, position))},
        SDL_GPUVertexAttribute{1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                               static_cast<Uint32>(offsetof(hover::render::Vertex, normal))},
        SDL_GPUVertexAttribute{2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                               static_cast<Uint32>(offsetof(hover::render::Vertex, color))},
    };

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.vertex_shader = vertex_shader.get();
    pipeline_info.fragment_shader = fragment_shader.get();
    pipeline_info.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_description;
    pipeline_info.vertex_input_state.num_vertex_buffers = 1;
    pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes.data();
    pipeline_info.vertex_input_state.num_vertex_attributes =
        static_cast<Uint32>(vertex_attributes.size());
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.rasterizer_state.enable_depth_clip = true;
    pipeline_info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    pipeline_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    pipeline_info.depth_stencil_state.enable_depth_test = true;
    pipeline_info.depth_stencil_state.enable_depth_write = true;
    pipeline_info.target_info.color_target_descriptions = &color_target;
    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    pipeline_info.target_info.has_depth_stencil_target = true;

    GraphicsPipeline pipeline{SDL_CreateGPUGraphicsPipeline(device, &pipeline_info),
                              GraphicsPipelineDeleter{device}};
    if (!pipeline) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create the vehicle pipeline: %s",
                     SDL_GetError());
    } else {
        SDL_Log("3D vehicle pipeline created from SPIR-V shaders in '%s'.",
                shader_directory.c_str());
    }

    return pipeline;
}

void draw_mesh(SDL_GPURenderPass* render_pass, const hover::render::GpuMesh& mesh) {
    const SDL_GPUBufferBinding vertex_binding{mesh.vertex_buffer(), 0};
    const SDL_GPUBufferBinding index_binding{mesh.index_buffer(), 0};
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);
    SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_DrawGPUIndexedPrimitives(render_pass, mesh.index_count(), 1, 0, 0, 0);
}

struct SceneMeshes {
    const hover::render::GpuMesh& ship;
    const hover::render::GpuMesh& presentation_pad;
};

RenderResult render_frame(SDL_GPUDevice* device, SDL_Window* window,
                          SDL_GPUGraphicsPipeline* pipeline, const SceneMeshes& meshes,
                          DepthTarget& depth_target) {
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (command_buffer == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not acquire a GPU command buffer: %s",
                     SDL_GetError());
        return RenderResult::failed;
    }

    SDL_GPUTexture* swapchain_texture = nullptr;
    Uint32 swapchain_width = 0;
    Uint32 swapchain_height = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture,
                                               &swapchain_width, &swapchain_height)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not acquire the swapchain texture: %s",
                     SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer);
        return RenderResult::failed;
    }

    if (swapchain_texture == nullptr) {
        if (!SDL_CancelGPUCommandBuffer(command_buffer)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Could not cancel an unused command buffer: %s", SDL_GetError());
            return RenderResult::failed;
        }

        SDL_Delay(10);
        return RenderResult::skipped;
    }

    if (!depth_target.ensure_size(swapchain_width, swapchain_height)) {
        if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Could not submit the command buffer after a depth-target failure: %s",
                         SDL_GetError());
        }
        return RenderResult::failed;
    }

    const float aspect_ratio =
        static_cast<float>(swapchain_width) / static_cast<float>(swapchain_height);
    const hover::math::Mat4 view = hover::math::look_at_lh(hover::math::LookAt{
        hover::math::Vec3{0.0F, 2.4F, -5.8F},
        hover::math::Vec3{0.0F, -0.05F, 0.15F},
        hover::math::Vec3{0.0F, 1.0F, 0.0F},
    });
    const hover::math::Mat4 projection = hover::math::perspective_lh(
        hover::math::Perspective{1.0471975512F, aspect_ratio, 0.1F, 100.0F});
    const hover::math::Mat4 view_projection = projection * view;
    static_assert(sizeof(hover::math::Mat4) == sizeof(float) * 16);
    SDL_PushGPUVertexUniformData(command_buffer, 0, view_projection.elements.data(),
                                 static_cast<Uint32>(sizeof(view_projection)));

    SDL_GPUColorTargetInfo color_target{};
    color_target.texture = swapchain_texture;
    color_target.clear_color = SDL_FColor{0.04F, 0.12F, 0.22F, 1.0F};
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depth_target_info{};
    depth_target_info.texture = depth_target.texture();
    depth_target_info.clear_depth = 1.0F;
    depth_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* render_pass =
        SDL_BeginGPURenderPass(command_buffer, &color_target, 1, &depth_target_info);
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
    draw_mesh(render_pass, meshes.presentation_pad);
    draw_mesh(render_pass, meshes.ship);
    SDL_EndGPURenderPass(render_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not submit the GPU command buffer: %s",
                     SDL_GetError());
        return RenderResult::failed;
    }

    return RenderResult::presented;
}

int run() {
    Window window{SDL_CreateWindow("Codename Hover", initial_window_width, initial_window_height,
                                   SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY)};
    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create the game window: %s",
                     SDL_GetError());
        return EXIT_FAILURE;
    }

    GpuDevice gpu_device{SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, gpu_debug_mode, nullptr)};
    if (!gpu_device) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create the SDL_GPU device: %s",
                     SDL_GetError());
        return EXIT_FAILURE;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu_device.get(), window.get())) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not claim the window for SDL_GPU: %s",
                     SDL_GetError());
        return EXIT_FAILURE;
    }
    const GpuWindowClaim gpu_window_claim{gpu_device.get(), window.get()};

    GraphicsPipeline vehicle_pipeline = create_vehicle_pipeline(gpu_device.get(), window.get());
    if (!vehicle_pipeline) {
        return EXIT_FAILURE;
    }

    const hover::game::ShipDefinition& ship_definition =
        hover::game::ships::prototype_01_definition();
    if (!hover::game::is_valid(ship_definition)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Ship definition '%.*s' is invalid.",
                     static_cast<int>(ship_definition.id.size()), ship_definition.id.data());
        return EXIT_FAILURE;
    }

    hover::render::GpuMesh ship_mesh{gpu_device.get()};
    if (!ship_mesh.upload(hover::assets::generated::make_prototype_01_mesh(),
                          ship_definition.visual_mesh_id)) {
        return EXIT_FAILURE;
    }

    hover::render::GpuMesh presentation_pad{gpu_device.get()};
    if (!presentation_pad.upload(hover::assets::generated::make_presentation_pad_mesh(),
                                 "generated/presentation_pad")) {
        return EXIT_FAILURE;
    }
    const SceneMeshes scene_meshes{ship_mesh, presentation_pad};
    DepthTarget depth_target{gpu_device.get()};

    SDL_Log("Loaded ship '%.*s': %.0f m/s top speed, %.0f energy, %.2f relative mass.",
            static_cast<int>(ship_definition.display_name.size()),
            ship_definition.display_name.data(),
            ship_definition.handling.maximum_forward_speed_metres_per_second,
            ship_definition.collision.maximum_energy, ship_definition.collision.relative_mass);

    const char* video_driver = SDL_GetCurrentVideoDriver();
    SDL_Log("Window created with the %s video driver. Press Escape or close the window to exit.",
            video_driver != nullptr ? video_driver : "unknown");

    const char* gpu_driver = SDL_GetGPUDeviceDriver(gpu_device.get());
    SDL_Log("GPU device created with the %s backend; debug validation is %s.",
            gpu_driver != nullptr ? gpu_driver : "unknown",
            gpu_debug_mode ? "enabled" : "disabled");

    FrameStatistics frame_statistics{window.get()};
    bool running = true;
    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (requests_exit(event)) {
                running = false;
            }
        }

        if (running) {
            const RenderResult render_result = render_frame(
                gpu_device.get(), window.get(), vehicle_pipeline.get(), scene_meshes, depth_target);
            if (render_result == RenderResult::failed) {
                return EXIT_FAILURE;
            }
            if (render_result == RenderResult::skipped) {
                frame_statistics.reset_after_skipped_frame();
            } else {
                frame_statistics.record_presented_frame();
            }
        }
    }

    SDL_Log("Shutting down cleanly.");
    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char* argv[]) {
    static_cast<void>(argc);
    static_cast<void>(argv);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    const int result = run();
    SDL_Quit();
    return result;
}
