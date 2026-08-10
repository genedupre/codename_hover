#include <SDL3/SDL.h>

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

    GpuShader shader{SDL_CreateGPUShader(device, &shader_info), GpuShaderDeleter{device}};
    if (!shader) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create shader '%s': %s", path.c_str(),
                     SDL_GetError());
    }

    return shader;
}

GraphicsPipeline create_triangle_pipeline(SDL_GPUDevice* device, SDL_Window* window) {
    const char* base_path = SDL_GetBasePath();
    if (base_path == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not find the executable directory: %s",
                     SDL_GetError());
        return GraphicsPipeline{nullptr, GraphicsPipelineDeleter{device}};
    }

    const std::string shader_directory = std::string{base_path} + "shaders/";
    GpuShader vertex_shader =
        load_shader(device, shader_directory + "triangle_vertex.spv", SDL_GPU_SHADERSTAGE_VERTEX);
    if (!vertex_shader) {
        return GraphicsPipeline{nullptr, GraphicsPipelineDeleter{device}};
    }

    GpuShader fragment_shader = load_shader(device, shader_directory + "triangle_fragment.spv",
                                            SDL_GPU_SHADERSTAGE_FRAGMENT);
    if (!fragment_shader) {
        return GraphicsPipeline{nullptr, GraphicsPipelineDeleter{device}};
    }

    SDL_GPUColorTargetDescription color_target{};
    color_target.format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.vertex_shader = vertex_shader.get();
    pipeline_info.fragment_shader = fragment_shader.get();
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.rasterizer_state.enable_depth_clip = true;
    pipeline_info.target_info.color_target_descriptions = &color_target;
    pipeline_info.target_info.num_color_targets = 1;

    GraphicsPipeline pipeline{SDL_CreateGPUGraphicsPipeline(device, &pipeline_info),
                              GraphicsPipelineDeleter{device}};
    if (!pipeline) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create the triangle pipeline: %s",
                     SDL_GetError());
    } else {
        SDL_Log("Triangle pipeline created from SPIR-V shaders in '%s'.", shader_directory.c_str());
    }

    return pipeline;
}

RenderResult render_frame(SDL_GPUDevice* device, SDL_Window* window,
                          SDL_GPUGraphicsPipeline* pipeline) {
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (command_buffer == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not acquire a GPU command buffer: %s",
                     SDL_GetError());
        return RenderResult::failed;
    }

    SDL_GPUTexture* swapchain_texture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr,
                                               nullptr)) {
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

    SDL_GPUColorTargetInfo color_target{};
    color_target.texture = swapchain_texture;
    color_target.clear_color = SDL_FColor{0.04F, 0.12F, 0.22F, 1.0F};
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* render_pass =
        SDL_BeginGPURenderPass(command_buffer, &color_target, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
    SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
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

    GraphicsPipeline triangle_pipeline = create_triangle_pipeline(gpu_device.get(), window.get());
    if (!triangle_pipeline) {
        return EXIT_FAILURE;
    }

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
            const RenderResult render_result =
                render_frame(gpu_device.get(), window.get(), triangle_pipeline.get());
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
