#include <SDL3/SDL.h>

#include "assets/generated/engine_pulse_mesh.hpp"
#include "assets/generated/presentation_pad.hpp"
#include "assets/generated/prototype_01_mesh.hpp"
#include "assets/generated/track_surface_mesh.hpp"
#include "core/fixed_step.hpp"
#include "core/launch_options.hpp"
#include "game/ships/prototype_01.hpp"
#include "game/track_vehicle_simulation.hpp"
#include "game/tracks/oval_track.hpp"
#include "game/tracks/speedway_track.hpp"
#include "game/vehicle_simulation.hpp"
#include "game/world_track_vehicle_simulation.hpp"
#include "hover_math.hpp"
#include "input/player_input.hpp"
#include "platform/sdl_input.hpp"
#include "render/engine_pulse.hpp"
#include "render/gpu_mesh.hpp"
#include "render/vehicle_presentation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr int initial_window_width = 1280;
constexpr int initial_window_height = 720;

const char* contact_mode_name(hover::game::VehicleContactMode mode) {
    switch (mode) {
    case hover::game::VehicleContactMode::supported:
        return "supported";
    case hover::game::VehicleContactMode::airborne:
        return "airborne";
    case hover::game::VehicleContactMode::falling:
        return "falling";
    case hover::game::VehicleContactMode::crashed:
        return "crashed";
    }
    return "unknown";
}

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

    void record_presented_frame(
        float speed_metres_per_second, const hover::input::PlayerInput& input,
        const hover::game::WorldTrackVehicleTelemetry* handling_telemetry = nullptr) {
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

        if (handling_telemetry != nullptr) {
            SDL_Log(
                "Frame timing: %.1f FPS | %.3f ms average | %.3f ms worst | %.1f km/h | "
                "steer %.2f throttle %.2f brake %.2f drift L%s/R%s boost %s | "
                "lateral %.2f m/s slip %.2f deg direction %.4f rad drift force %.2f "
                "sustained %.2f propulsion %.2f m/s^2 | grip %.1f/%.1f sat %.2f "
                "height %.2f normal %.2f contact %s",
                frames_per_second, average_frame_ms, worst_frame_ms,
                static_cast<double>(speed_metres_per_second) * 3.6,
                static_cast<double>(input.steering), static_cast<double>(input.throttle),
                static_cast<double>(input.brake), input.drift_left ? "on" : "off",
                input.drift_right ? "on" : "off", input.boost ? "on" : "off",
                static_cast<double>(handling_telemetry->local_lateral_speed_metres_per_second),
                static_cast<double>(handling_telemetry->signed_slip_angle_radians) * 180.0 /
                    std::numbers::pi,
                static_cast<double>(handling_telemetry->steering_direction_change_radians),
                static_cast<double>(handling_telemetry->drift_force_fraction),
                static_cast<double>(handling_telemetry->sustained_slip_intensity),
                static_cast<double>(
                    handling_telemetry->applied_propulsion_acceleration_metres_per_second_squared),
                static_cast<double>(
                    handling_telemetry->available_grip_deceleration_metres_per_second_squared),
                static_cast<double>(
                    handling_telemetry->grip_demand_deceleration_metres_per_second_squared),
                static_cast<double>(handling_telemetry->traction_saturation_ratio),
                static_cast<double>(handling_telemetry->height_above_surface_metres),
                static_cast<double>(handling_telemetry->surface_normal_speed_metres_per_second),
                contact_mode_name(handling_telemetry->contact_mode));
        } else {
            SDL_Log("Frame timing: %.1f FPS | %.3f ms average | %.3f ms worst | %.1f km/h | "
                    "steer %.2f throttle %.2f brake %.2f drift L%s/R%s boost %s",
                    frames_per_second, average_frame_ms, worst_frame_ms,
                    static_cast<double>(speed_metres_per_second) * 3.6,
                    static_cast<double>(input.steering), static_cast<double>(input.throttle),
                    static_cast<double>(input.brake), input.drift_left ? "on" : "off",
                    input.drift_right ? "on" : "off", input.boost ? "on" : "off");
        }

        if (title_updates_enabled_) {
            char title[192]{};
            SDL_snprintf(
                title, sizeof(title),
                "Codename Hover | %.1f FPS | %.2f ms | %.0f km/h | S %.2f T %.2f B %.2f%s%s%s",
                frames_per_second, average_frame_ms,
                static_cast<double>(speed_metres_per_second) * 3.6,
                static_cast<double>(input.steering), static_cast<double>(input.throttle),
                static_cast<double>(input.brake), input.drift_left ? " | DRIFT L" : "",
                input.drift_right ? " | DRIFT R" : "", input.boost ? " | BOOST" : "");
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

class FrameTimer final {
  public:
    FrameTimer()
        : performance_frequency_(static_cast<double>(SDL_GetPerformanceFrequency())),
          previous_counter_(SDL_GetPerformanceCounter()) {}

    [[nodiscard]] double elapsed_seconds() {
        const Uint64 current_counter = SDL_GetPerformanceCounter();
        const double elapsed =
            static_cast<double>(current_counter - previous_counter_) / performance_frequency_;
        previous_counter_ = current_counter;
        return elapsed;
    }

    void reset() { previous_counter_ = SDL_GetPerformanceCounter(); }

  private:
    double performance_frequency_;
    Uint64 previous_counter_;
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

struct MeshPipelineConfig {
    std::string_view fragment_shader_filename;
    std::string_view debug_name;
    bool alpha_blending;
    bool depth_write;
};

GraphicsPipeline create_mesh_pipeline(SDL_GPUDevice* device, SDL_Window* window,
                                      MeshPipelineConfig config) {
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

    GpuShader fragment_shader =
        load_shader(device, shader_directory + std::string{config.fragment_shader_filename},
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
        SDL_GPUVertexAttribute{3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
                               static_cast<Uint32>(offsetof(hover::render::Vertex, opacity))},
    };

    if (config.alpha_blending) {
        color_target.blend_state.enable_blend = true;
        color_target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        color_target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        color_target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        color_target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        color_target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        color_target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    }

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
    pipeline_info.depth_stencil_state.enable_depth_write = config.depth_write;
    pipeline_info.target_info.color_target_descriptions = &color_target;
    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    pipeline_info.target_info.has_depth_stencil_target = true;

    GraphicsPipeline pipeline{SDL_CreateGPUGraphicsPipeline(device, &pipeline_info),
                              GraphicsPipelineDeleter{device}};
    if (!pipeline) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create the %.*s pipeline: %s",
                     static_cast<int>(config.debug_name.size()), config.debug_name.data(),
                     SDL_GetError());
    } else {
        SDL_Log("%.*s pipeline created from SPIR-V shaders in '%s'.",
                static_cast<int>(config.debug_name.size()), config.debug_name.data(),
                shader_directory.c_str());
    }

    return pipeline;
}

struct VertexUniforms {
    hover::math::Mat4 view_projection;
    hover::math::Mat4 model;
};

static_assert(sizeof(VertexUniforms) == sizeof(float) * 32);

void draw_mesh(SDL_GPUCommandBuffer* command_buffer, SDL_GPURenderPass* render_pass,
               const hover::render::GpuMesh& mesh, const VertexUniforms& uniforms) {
    SDL_PushGPUVertexUniformData(command_buffer, 0, &uniforms,
                                 static_cast<Uint32>(sizeof(uniforms)));
    const SDL_GPUBufferBinding vertex_binding{mesh.vertex_buffer(), 0};
    const SDL_GPUBufferBinding index_binding{mesh.index_buffer(), 0};
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);
    SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_DrawGPUIndexedPrimitives(render_pass, mesh.index_count(), 1, 0, 0, 0);
}

struct SceneMeshes {
    const hover::render::GpuMesh& ship;
    const hover::render::GpuMesh& canopy;
    const hover::render::GpuMesh& driver;
    const hover::render::GpuMesh& engine_pulse_outer;
    const hover::render::GpuMesh& engine_pulse_core;
    const hover::render::GpuMesh& engine_boost_flare;
    const hover::render::GpuMesh& world;
};

enum class ScenarioMovement : std::uint8_t {
    planar,
    scalar_track,
    world_track,
};

struct ScenarioSetup {
    hover::render::MeshData world_mesh;
    std::string_view world_mesh_id;
    hover::game::VehicleState initial_vehicle_state;
    std::optional<hover::game::SampledTrack> driving_track;
    ScenarioMovement movement;
};

ScenarioSetup
make_track_scenario_setup(hover::game::SampledTrack track, std::string_view world_mesh_id,
                          ScenarioMovement movement = ScenarioMovement::scalar_track) {
    hover::render::MeshData world_mesh = hover::assets::generated::make_track_surface_mesh(track);
    return ScenarioSetup{
        std::move(world_mesh), world_mesh_id, {}, std::move(track), movement,
    };
}

ScenarioSetup make_scenario_setup(hover::core::DevelopmentScenario scenario) {
    switch (scenario) {
    case hover::core::DevelopmentScenario::runway:
        return ScenarioSetup{
            hover::assets::generated::make_presentation_pad_mesh(),
            "generated/presentation_pad",
            {},
            std::nullopt,
            ScenarioMovement::planar,
        };
    case hover::core::DevelopmentScenario::oval: {
        constexpr hover::game::tracks::OvalTrackDefinition oval_definition{
            .straight_length_metres = 600.0F,
            .turn_radius_metres = 180.0F,
            .half_width_metres = 24.0F,
            .elevation_metres = -0.62F,
        };
        constexpr std::uint32_t sample_count = 512U;
        const hover::game::SampledTrack track = hover::game::tracks::make_sampled_oval(
            hover::game::tracks::OvalTrackBuild{oval_definition, sample_count});
        return make_track_scenario_setup(track, "generated/oval_track_surface");
    }
    case hover::core::DevelopmentScenario::speedway:
    case hover::core::DevelopmentScenario::speedway_physics: {
        hover::game::tracks::SpeedwayTrackDefinition speedway_definition{
            .oval =
                {
                    .straight_length_metres = 600.0F,
                    .turn_radius_metres = 180.0F,
                    .half_width_metres = 24.0F,
                    .elevation_metres = -0.62F,
                },
            .maximum_bank_radians = 0.4886921906F,
            .bank_transition_metres = 85.0F,
        };
        if (scenario == hover::core::DevelopmentScenario::speedway_physics) {
            speedway_definition.second_turn_properties = {
                .left_edge = hover::game::TrackEdgePolicy::open,
                .right_edge = hover::game::TrackEdgePolicy::open,
            };
        }
        constexpr std::uint32_t sample_count = 512U;
        const hover::game::SampledTrack track = hover::game::tracks::make_sampled_speedway(
            hover::game::tracks::SpeedwayTrackBuild{speedway_definition, sample_count});
        const ScenarioMovement movement =
            scenario == hover::core::DevelopmentScenario::speedway_physics
                ? ScenarioMovement::world_track
                : ScenarioMovement::scalar_track;
        return make_track_scenario_setup(track, "generated/speedway_track_surface", movement);
    }
    case hover::core::DevelopmentScenario::handling_lab: {
        constexpr hover::game::tracks::OvalTrackDefinition handling_lab_definition{
            .straight_length_metres = 6'000.0F,
            .turn_radius_metres = 1'000.0F,
            .half_width_metres = 800.0F,
            .elevation_metres = -0.62F,
        };
        constexpr std::uint32_t sample_count = 2'048U;
        const hover::game::SampledTrack track = hover::game::tracks::make_sampled_oval(
            hover::game::tracks::OvalTrackBuild{handling_lab_definition, sample_count});
        return make_track_scenario_setup(track, "generated/handling_lab_surface",
                                         ScenarioMovement::world_track);
    }
    }

    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unknown development scenario.");
    std::abort();
}

RenderResult
render_frame(SDL_GPUDevice* device, SDL_Window* window, SDL_GPUGraphicsPipeline* opaque_pipeline,
             SDL_GPUGraphicsPipeline* transparent_vehicle_pipeline,
             SDL_GPUGraphicsPipeline* pulse_pipeline, const SceneMeshes& meshes,
             const hover::game::VehiclePose& ship_pose,
             hover::render::EnginePulseSample engine_pulse, hover::math::Vec3 vehicle_vibration,
             hover::render::BoostCameraSample boost_camera, DepthTarget& depth_target) {
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
    const hover::math::Vec3 ship_forward = hover::game::forward_direction(ship_pose);
    const hover::math::Vec3 ship_up = hover::game::up_direction(ship_pose);
    const hover::math::Mat4 view = hover::math::look_at_lh(hover::math::LookAt{
        ship_pose.position - ship_forward * boost_camera.follow_distance_metres + ship_up * 3.8F,
        ship_pose.position + ship_forward * boost_camera.look_ahead_metres + ship_up * 0.35F,
        ship_up,
    });
    const hover::math::Mat4 projection = hover::math::perspective_lh(hover::math::Perspective{
        boost_camera.vertical_field_of_view_radians, aspect_ratio, 0.1F, 400.0F});
    const hover::math::Mat4 view_projection = projection * view;

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
    SDL_BindGPUGraphicsPipeline(render_pass, opaque_pipeline);
    draw_mesh(command_buffer, render_pass, meshes.world,
              VertexUniforms{view_projection, hover::math::identity()});
    const hover::math::Mat4 ship_model =
        hover::game::model_matrix(ship_pose) * hover::math::translation(vehicle_vibration);
    draw_mesh(command_buffer, render_pass, meshes.ship,
              VertexUniforms{view_projection, ship_model});
    draw_mesh(command_buffer, render_pass, meshes.driver,
              VertexUniforms{view_projection, ship_model});
    SDL_BindGPUGraphicsPipeline(render_pass, transparent_vehicle_pipeline);
    draw_mesh(command_buffer, render_pass, meshes.canopy,
              VertexUniforms{view_projection, ship_model});
    if (engine_pulse.visible) {
        constexpr std::array engine_sockets{
            hover::math::Vec3{0.72F, -0.09F, -2.50F},
            hover::math::Vec3{-0.72F, -0.09F, -2.50F},
        };
        SDL_BindGPUGraphicsPipeline(render_pass, pulse_pipeline);
        for (const hover::math::Vec3 socket : engine_sockets) {
            if (engine_pulse.boost_flare_visible) {
                const hover::math::Mat4 boost_flare_model =
                    ship_model * hover::math::translation(socket) *
                    hover::math::scaling(engine_pulse.boost_flare_scale);
                draw_mesh(command_buffer, render_pass, meshes.engine_boost_flare,
                          VertexUniforms{view_projection, boost_flare_model});
            }
            const hover::math::Mat4 pulse_model =
                ship_model * hover::math::translation(socket) *
                hover::math::scaling(hover::math::Vec3{engine_pulse.radial_scale,
                                                       engine_pulse.radial_scale,
                                                       engine_pulse.length_scale});
            draw_mesh(command_buffer, render_pass, meshes.engine_pulse_outer,
                      VertexUniforms{view_projection, pulse_model});
            draw_mesh(command_buffer, render_pass, meshes.engine_pulse_core,
                      VertexUniforms{view_projection, pulse_model});
        }
    }
    SDL_EndGPURenderPass(render_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not submit the GPU command buffer: %s",
                     SDL_GetError());
        return RenderResult::failed;
    }

    return RenderResult::presented;
}

int run(hover::core::DevelopmentScenario scenario) {
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

    GraphicsPipeline vehicle_pipeline =
        create_mesh_pipeline(gpu_device.get(), window.get(),
                             MeshPipelineConfig{"vehicle_fragment.spv", "opaque 3D", false, true});
    if (!vehicle_pipeline) {
        return EXIT_FAILURE;
    }
    GraphicsPipeline engine_pulse_pipeline = create_mesh_pipeline(
        gpu_device.get(), window.get(),
        MeshPipelineConfig{"engine_pulse_fragment.spv", "blended engine pulse", true, false});
    if (!engine_pulse_pipeline) {
        return EXIT_FAILURE;
    }
    GraphicsPipeline transparent_vehicle_pipeline = create_mesh_pipeline(
        gpu_device.get(), window.get(),
        MeshPipelineConfig{"vehicle_fragment.spv", "transparent 3D", true, false});
    if (!transparent_vehicle_pipeline) {
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
    hover::render::GpuMesh canopy_mesh{gpu_device.get()};
    if (!canopy_mesh.upload(hover::assets::generated::make_prototype_01_canopy_mesh(),
                            "generated/prototype_01_canopy")) {
        return EXIT_FAILURE;
    }
    hover::render::GpuMesh driver_mesh{gpu_device.get()};
    if (!driver_mesh.upload(hover::assets::generated::make_prototype_01_driver_mesh(),
                            "generated/prototype_01_driver")) {
        return EXIT_FAILURE;
    }

    hover::render::GpuMesh engine_pulse_outer_mesh{gpu_device.get()};
    if (!engine_pulse_outer_mesh.upload(hover::assets::generated::make_engine_pulse_outer_mesh(),
                                        "generated/prototype_01_engine_pulse_outer")) {
        return EXIT_FAILURE;
    }
    hover::render::GpuMesh engine_pulse_core_mesh{gpu_device.get()};
    if (!engine_pulse_core_mesh.upload(hover::assets::generated::make_engine_pulse_core_mesh(),
                                       "generated/prototype_01_engine_pulse_core")) {
        return EXIT_FAILURE;
    }
    hover::render::GpuMesh engine_boost_flare_mesh{gpu_device.get()};
    if (!engine_boost_flare_mesh.upload(hover::assets::generated::make_engine_boost_flare_mesh(),
                                        "generated/prototype_01_engine_boost_flare")) {
        return EXIT_FAILURE;
    }

    const ScenarioSetup scenario_setup = make_scenario_setup(scenario);
    hover::render::GpuMesh world_mesh{gpu_device.get()};
    if (!world_mesh.upload(scenario_setup.world_mesh, scenario_setup.world_mesh_id)) {
        return EXIT_FAILURE;
    }
    const SceneMeshes scene_meshes{ship_mesh,
                                   canopy_mesh,
                                   driver_mesh,
                                   engine_pulse_outer_mesh,
                                   engine_pulse_core_mesh,
                                   engine_boost_flare_mesh,
                                   world_mesh};
    DepthTarget depth_target{gpu_device.get()};

    const std::string_view active_scenario = hover::core::scenario_name(scenario);
    SDL_Log("Development scenario: %.*s.", static_cast<int>(active_scenario.size()),
            active_scenario.data());

    hover::platform::SdlInput input_system;
    if (!input_system.initialize()) {
        return EXIT_FAILURE;
    }

    SDL_Log("Loaded ship '%.*s': %.0f m/s top speed, %.0f energy, %.2f relative mass.",
            static_cast<int>(ship_definition.display_name.size()),
            ship_definition.display_name.data(),
            ship_definition.handling.base_maximum_forward_speed_metres_per_second,
            ship_definition.collision.maximum_energy, ship_definition.collision.relative_mass);

    const char* video_driver = SDL_GetCurrentVideoDriver();
    SDL_Log("Window created with the %s video driver. Press Escape or close the window to exit.",
            video_driver != nullptr ? video_driver : "unknown");

    const char* gpu_driver = SDL_GetGPUDeviceDriver(gpu_device.get());
    SDL_Log("GPU device created with the %s backend; debug validation is %s.",
            gpu_driver != nullptr ? gpu_driver : "unknown",
            gpu_debug_mode ? "enabled" : "disabled");

    hover::core::FixedStepAccumulator simulation_clock{hover::core::FixedStepConfig{
        hover::core::simulation_tick_seconds,
        0.25,
        8,
    }};
    FrameTimer frame_timer;
    FrameStatistics frame_statistics{window.get()};
    hover::game::VehicleState previous_vehicle_state = scenario_setup.initial_vehicle_state;
    hover::game::VehicleState current_vehicle_state = scenario_setup.initial_vehicle_state;
    constexpr hover::game::TrackPathId primary_path_id{1U};
    std::optional<hover::game::TrackVehicleState> track_vehicle_state;
    std::optional<hover::game::WorldTrackVehicleState> world_track_vehicle_state;
    std::optional<hover::game::WorldTrackVehicleTelemetry> world_track_telemetry;
    if (scenario_setup.driving_track.has_value() &&
        scenario_setup.movement == ScenarioMovement::scalar_track) {
        track_vehicle_state = hover::game::make_track_vehicle_state(
            {}, ship_definition,
            hover::game::ResolvedTrackPath{primary_path_id, *scenario_setup.driving_track});
        previous_vehicle_state = track_vehicle_state->vehicle;
        current_vehicle_state = track_vehicle_state->vehicle;
    } else if (scenario_setup.driving_track.has_value() &&
               scenario_setup.movement == ScenarioMovement::world_track) {
        world_track_vehicle_state = hover::game::make_world_track_vehicle_state(
            {}, ship_definition,
            hover::game::ResolvedTrackPath{primary_path_id, *scenario_setup.driving_track});
        previous_vehicle_state = world_track_vehicle_state->vehicle;
        current_vehicle_state = world_track_vehicle_state->vehicle;
    }
    double engine_pulse_elapsed_seconds = 0.0;
    float engine_pulse_intensity = 0.0F;
    hover::render::BoostCameraFeedbackState boost_camera_feedback{};
    constexpr hover::platform::SdlInput::RumbleEffect boost_activation_rumble{
        .low_frequency = 0.35F,
        .high_frequency = 0.80F,
        .duration_ms = 160,
    };
    bool running = true;
    while (running) {
        const double elapsed_seconds = frame_timer.elapsed_seconds();
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (input_system.handle_event(event)) {
                running = false;
            }
        }

        if (running) {
            const hover::input::PlayerInput player_input = input_system.sample_player_one();
            engine_pulse_elapsed_seconds += elapsed_seconds;
            const hover::core::FixedStepPlan step_plan = simulation_clock.advance(elapsed_seconds);
            if (step_plan.dropped_time) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Simulation catch-up limit reached; excess accumulated time dropped.");
            }
            bool boost_activated_this_frame = false;
            float wall_impact_speed_this_frame = 0.0F;
            for (std::uint32_t tick = 0; tick < step_plan.tick_count; ++tick) {
                previous_vehicle_state = current_vehicle_state;
                hover::game::VehicleTickEvents events{};
                if (world_track_vehicle_state.has_value()) {
                    const hover::game::WorldTrackVehicleTickResult result =
                        hover::game::simulate_world_track_vehicle(
                            *world_track_vehicle_state,
                            hover::game::WorldTrackVehicleTick{
                                player_input,
                                ship_definition,
                                hover::game::ResolvedTrackPath{primary_path_id,
                                                               *scenario_setup.driving_track},
                                static_cast<float>(hover::core::simulation_tick_seconds),
                            });
                    events.boost_activated = result.events.boost_activated;
                    wall_impact_speed_this_frame =
                        std::max(wall_impact_speed_this_frame,
                                 result.events.wall_impact_speed_metres_per_second);
                    world_track_telemetry = result.telemetry;
                    current_vehicle_state = world_track_vehicle_state->vehicle;
                } else if (track_vehicle_state.has_value()) {
                    events = hover::game::simulate_track_vehicle(
                        *track_vehicle_state,
                        hover::game::TrackVehicleTick{
                            player_input,
                            ship_definition,
                            hover::game::ResolvedTrackPath{primary_path_id,
                                                           *scenario_setup.driving_track},
                            static_cast<float>(hover::core::simulation_tick_seconds),
                        });
                    current_vehicle_state = track_vehicle_state->vehicle;
                } else {
                    events = hover::game::simulate_vehicle(
                        current_vehicle_state,
                        hover::game::VehicleTick{
                            player_input, ship_definition,
                            static_cast<float>(hover::core::simulation_tick_seconds)});
                }
                boost_activated_this_frame = boost_activated_this_frame || events.boost_activated;
            }
            if (boost_activated_this_frame) {
                input_system.rumble_all(boost_activation_rumble);
            }
            if (wall_impact_speed_this_frame > 0.0F) {
                const float impact_fraction = std::clamp(
                    wall_impact_speed_this_frame /
                        ship_definition.handling.base_maximum_forward_speed_metres_per_second,
                    0.0F, 1.0F);
                input_system.rumble_all({
                    .low_frequency = 0.18F + 0.52F * impact_fraction,
                    .high_frequency = 0.12F + 0.58F * impact_fraction,
                    .duration_ms = 100,
                });
            }
            const hover::game::VehiclePose render_pose =
                hover::game::interpolate(previous_vehicle_state.pose, current_vehicle_state.pose,
                                         step_plan.interpolation_alpha);
            const hover::game::HandlingProfile& handling = ship_definition.handling;
            const float propulsion_acceleration =
                player_input.throttle * handling.forward_acceleration_metres_per_second_squared -
                player_input.brake * handling.braking_deceleration_metres_per_second_squared;
            const float propulsion_intensity = std::clamp(
                propulsion_acceleration / handling.forward_acceleration_metres_per_second_squared,
                0.0F, 1.0F);
            const float requested_engine_intensity =
                current_vehicle_state.boosting ? 1.0F : propulsion_intensity;
            engine_pulse_intensity = hover::render::advance_engine_pulse_intensity(
                engine_pulse_intensity, requested_engine_intensity, elapsed_seconds);
            const float speed_ratio = current_vehicle_state.forward_speed_metres_per_second /
                                      handling.base_maximum_forward_speed_metres_per_second;
            const hover::render::EnginePulseSample engine_pulse =
                hover::render::sample_engine_pulse(engine_pulse_elapsed_seconds,
                                                   engine_pulse_intensity, speed_ratio,
                                                   current_vehicle_state.boosting);
            const hover::math::Vec3 vehicle_vibration = hover::render::sample_full_speed_vibration(
                engine_pulse_elapsed_seconds, speed_ratio);
            hover::render::advance_boost_camera_feedback(boost_camera_feedback, elapsed_seconds,
                                                         current_vehicle_state.boosting,
                                                         speed_ratio);
            const hover::render::BoostCameraSample boost_camera =
                hover::render::sample_boost_camera(boost_camera_feedback.intensity);

            const RenderResult render_result = render_frame(
                gpu_device.get(), window.get(), vehicle_pipeline.get(),
                transparent_vehicle_pipeline.get(), engine_pulse_pipeline.get(), scene_meshes,
                render_pose, engine_pulse, vehicle_vibration, boost_camera, depth_target);
            if (render_result == RenderResult::failed) {
                return EXIT_FAILURE;
            }
            if (render_result == RenderResult::skipped) {
                frame_timer.reset();
                simulation_clock.reset();
                frame_statistics.reset_after_skipped_frame();
            } else {
                const hover::game::WorldTrackVehicleTelemetry* handling_telemetry =
                    scenario == hover::core::DevelopmentScenario::handling_lab &&
                            world_track_telemetry.has_value()
                        ? &*world_track_telemetry
                        : nullptr;
                frame_statistics.record_presented_frame(
                    current_vehicle_state.forward_speed_metres_per_second, player_input,
                    handling_telemetry);
            }
        }
    }

    SDL_Log("Shutting down cleanly.");
    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char* argv[]) {
    std::vector<std::string_view> arguments;
    arguments.reserve(argc > 1 ? static_cast<std::size_t>(argc - 1) : 0U);
    for (int index = 1; index < argc; ++index) {
        arguments.emplace_back(argv[index]);
    }

    const hover::core::LaunchOptionsParseResult parsed =
        hover::core::parse_launch_options(arguments);
    if (!parsed.succeeded()) {
        std::fprintf(stderr, "codename_hover: %s\nTry '--help' for usage.\n", parsed.error.c_str());
        return EXIT_FAILURE;
    }
    if (parsed.options.show_help) {
        std::fputs("Usage: codename_hover [--scenario NAME]\n"
                   "\n"
                   "Options:\n"
                   "  --scenario NAME   Start a named development scenario.\n"
                   "  --list-scenarios  List available scenarios without starting SDL.\n"
                   "  -h, --help        Show this help without starting SDL.\n",
                   stdout);
    }
    if (parsed.options.list_scenarios) {
        for (const hover::core::DevelopmentScenarioInfo& scenario :
             hover::core::development_scenarios()) {
            std::fprintf(stdout, "%.*s  %.*s\n", static_cast<int>(scenario.name.size()),
                         scenario.name.data(), static_cast<int>(scenario.description.size()),
                         scenario.description.data());
        }
    }
    if (parsed.options.show_help || parsed.options.list_scenarios) {
        return EXIT_SUCCESS;
    }

#if defined(SDL_PLATFORM_LINUX)
    if (SDL_GetHint(SDL_HINT_VIDEO_DRIVER) == nullptr &&
        !SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11")) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Could not set the Linux video-driver preference: %s", SDL_GetError());
    }
#endif

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    const int result = run(parsed.options.scenario);
    SDL_Quit();
    return result;
}
