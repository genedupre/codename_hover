#include <SDL3/SDL.h>

#include <cstdlib>
#include <memory>

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

bool render_clear_frame(SDL_GPUDevice* device, SDL_Window* window) {
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (command_buffer == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not acquire a GPU command buffer: %s",
                     SDL_GetError());
        return false;
    }

    SDL_GPUTexture* swapchain_texture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr,
                                               nullptr)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not acquire the swapchain texture: %s",
                     SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer);
        return false;
    }

    if (swapchain_texture == nullptr) {
        if (!SDL_CancelGPUCommandBuffer(command_buffer)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Could not cancel an unused command buffer: %s", SDL_GetError());
            return false;
        }

        SDL_Delay(10);
        return true;
    }

    SDL_GPUColorTargetInfo color_target{};
    color_target.texture = swapchain_texture;
    color_target.clear_color = SDL_FColor{0.04F, 0.12F, 0.22F, 1.0F};
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* render_pass =
        SDL_BeginGPURenderPass(command_buffer, &color_target, 1, nullptr);
    SDL_EndGPURenderPass(render_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not submit the GPU command buffer: %s",
                     SDL_GetError());
        return false;
    }

    return true;
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

    const char* video_driver = SDL_GetCurrentVideoDriver();
    SDL_Log("Window created with the %s video driver. Press Escape or close the window to exit.",
            video_driver != nullptr ? video_driver : "unknown");

    const char* gpu_driver = SDL_GetGPUDeviceDriver(gpu_device.get());
    SDL_Log("GPU device created with the %s backend; debug validation is %s.",
            gpu_driver != nullptr ? gpu_driver : "unknown",
            gpu_debug_mode ? "enabled" : "disabled");

    bool running = true;
    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (requests_exit(event)) {
                running = false;
            }
        }

        if (running && !render_clear_frame(gpu_device.get(), window.get())) {
            return EXIT_FAILURE;
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
