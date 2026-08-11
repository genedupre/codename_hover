#pragma once

#include "input/player_input.hpp"

#include <SDL3/SDL.h>

#include <cstddef>
#include <memory>
#include <vector>

namespace hover::platform {

class SdlInput final {
  public:
    struct RumbleEffect {
        float low_frequency;
        float high_frequency;
        Uint32 duration_ms;
    };

    SdlInput() = default;

    SdlInput(const SdlInput&) = delete;
    SdlInput& operator=(const SdlInput&) = delete;

    [[nodiscard]] bool initialize();
    [[nodiscard]] bool handle_event(const SDL_Event& event);
    [[nodiscard]] input::PlayerInput sample_player_one() const;
    void rumble_all(RumbleEffect effect);

    [[nodiscard]] std::size_t gamepad_count() const { return gamepads_.size(); }

  private:
    struct GamepadDeleter {
        void operator()(SDL_Gamepad* gamepad) const noexcept { SDL_CloseGamepad(gamepad); }
    };

    struct GamepadSlot {
        SDL_JoystickID instance_id;
        std::unique_ptr<SDL_Gamepad, GamepadDeleter> gamepad;
        bool supports_rumble;
    };

    void open_gamepad(SDL_JoystickID instance_id);
    void close_gamepad(SDL_JoystickID instance_id);
    void log_gamepad(const GamepadSlot& slot, const char* reason) const;

    std::vector<GamepadSlot> gamepads_;
};

} // namespace hover::platform
