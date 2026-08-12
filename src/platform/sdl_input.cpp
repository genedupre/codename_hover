#include "platform/sdl_input.hpp"

#include <algorithm>
#include <cmath>

namespace hover::platform {
namespace {

constexpr std::int16_t stick_dead_zone = 6000;
constexpr std::int16_t trigger_dead_zone = 1800;

input::PlayerInput sample_keyboard_and_mouse() {
    const bool* keys = SDL_GetKeyboardState(nullptr);
    const float keyboard_steering =
        static_cast<float>(keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) -
        static_cast<float>(keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]);

    input::PlayerInput result{};
    result.steering = keyboard_steering;
    result.throttle = static_cast<float>(keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]);
    result.brake = static_cast<float>(keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]);
    result.drift = keys[SDL_SCANCODE_SPACE];
    result.boost =
        keys[SDL_SCANCODE_X] || keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

    const SDL_MouseButtonFlags mouse_buttons = SDL_GetMouseState(nullptr, nullptr);
    result.throttle =
        std::max(result.throttle, static_cast<float>((mouse_buttons & SDL_BUTTON_LMASK) != 0U));
    result.brake =
        std::max(result.brake, static_cast<float>((mouse_buttons & SDL_BUTTON_RMASK) != 0U));
    result.drift = result.drift || (mouse_buttons & SDL_BUTTON_X1MASK) != 0U;
    result.boost = result.boost || (mouse_buttons & SDL_BUTTON_X2MASK) != 0U;
    return result;
}

input::PlayerInput sample_gamepad(SDL_Gamepad* gamepad) {
    input::PlayerInput analog{};
    analog.steering = input::normalize_signed_axis(
        input::AxisSample{SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX), stick_dead_zone});
    analog.brake = input::normalize_trigger(input::AxisSample{
        SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER), trigger_dead_zone});
    analog.drift = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);

    input::PlayerInput digital{};
    digital.steering =
        static_cast<float>(SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) -
        static_cast<float>(SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT));
    digital.throttle = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) ? 1.0F : 0.0F;
    digital.brake = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST) ? 1.0F : 0.0F;
    digital.boost = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST);
    return input::merge(analog, digital);
}

} // namespace

bool SdlInput::initialize() {
    SDL_ClearError();
    int gamepad_count = 0;
    SDL_JoystickID* gamepad_ids = SDL_GetGamepads(&gamepad_count);
    if (gamepad_ids == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_INPUT, "Could not enumerate startup gamepads: %s",
                    SDL_GetError());
        return true;
    }

    for (int index = 0; index < gamepad_count; ++index) {
        open_gamepad(gamepad_ids[index]);
    }
    SDL_free(gamepad_ids);

    SDL_Log("Input initialized with keyboard/mouse and %zu open gamepad(s).", gamepads_.size());
    return true;
}

bool SdlInput::handle_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_QUIT ||
        (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
        return true;
    }

    if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
        open_gamepad(event.gdevice.which);
    } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
        close_gamepad(event.gdevice.which);
    } else if (event.type == SDL_EVENT_GAMEPAD_REMAPPED ||
               event.type == SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED) {
        const auto slot =
            std::ranges::find(gamepads_, event.gdevice.which, &GamepadSlot::instance_id);
        if (slot != gamepads_.end()) {
            log_gamepad(*slot, event.type == SDL_EVENT_GAMEPAD_REMAPPED ? "remapped"
                                                                        : "Steam handle updated");
        }
    } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat && event.key.key == SDLK_R) {
        rumble_all(RumbleEffect{0.22F, 0.48F, 180});
    } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_BACK) {
            return true;
        }
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_NORTH) {
            rumble_all(RumbleEffect{0.22F, 0.48F, 180});
        }
    }
    return false;
}

input::PlayerInput SdlInput::sample_player_one() const {
    input::PlayerInput result = sample_keyboard_and_mouse();
    for (const GamepadSlot& slot : gamepads_) {
        result = input::merge(result, sample_gamepad(slot.gamepad.get()));
    }
    return result;
}

void SdlInput::rumble_all(RumbleEffect effect) {
    const Uint16 low = static_cast<Uint16>(std::clamp(effect.low_frequency, 0.0F, 1.0F) *
                                           static_cast<float>(SDL_MAX_UINT16));
    const Uint16 high = static_cast<Uint16>(std::clamp(effect.high_frequency, 0.0F, 1.0F) *
                                            static_cast<float>(SDL_MAX_UINT16));

    for (const GamepadSlot& slot : gamepads_) {
        if (!slot.supports_rumble) {
            continue;
        }
        if (!SDL_RumbleGamepad(slot.gamepad.get(), low, high, effect.duration_ms)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_INPUT, "Rumble failed for gamepad %u: %s",
                        slot.instance_id, SDL_GetError());
        }
    }
}

void SdlInput::open_gamepad(SDL_JoystickID instance_id) {
    if (std::ranges::find(gamepads_, instance_id, &GamepadSlot::instance_id) != gamepads_.end()) {
        return;
    }

    std::unique_ptr<SDL_Gamepad, GamepadDeleter> gamepad{SDL_OpenGamepad(instance_id)};
    if (!gamepad) {
        SDL_LogWarn(SDL_LOG_CATEGORY_INPUT, "Could not open gamepad %u: %s", instance_id,
                    SDL_GetError());
        return;
    }

    const SDL_PropertiesID properties = SDL_GetGamepadProperties(gamepad.get());
    const bool supports_rumble =
        properties != 0 &&
        SDL_GetBooleanProperty(properties, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
    gamepads_.push_back(GamepadSlot{instance_id, std::move(gamepad), supports_rumble});
    log_gamepad(gamepads_.back(), "opened");

    if (supports_rumble) {
        if (SDL_RumbleGamepad(gamepads_.back().gamepad.get(), 0x1800, 0x3000, 120)) {
            SDL_Log("Gamepad %u connection rumble submitted.", instance_id);
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_INPUT, "Connection rumble failed for gamepad %u: %s",
                        instance_id, SDL_GetError());
        }
    }
}

void SdlInput::close_gamepad(SDL_JoystickID instance_id) {
    const auto slot = std::ranges::find(gamepads_, instance_id, &GamepadSlot::instance_id);
    if (slot == gamepads_.end()) {
        return;
    }
    SDL_Log("Gamepad %u removed.", instance_id);
    gamepads_.erase(slot);
}

void SdlInput::log_gamepad(const GamepadSlot& slot, const char* reason) const {
    const char* name = SDL_GetGamepadName(slot.gamepad.get());
    const SDL_GamepadType type = SDL_GetGamepadType(slot.gamepad.get());
    const char* type_name = SDL_GetGamepadStringForType(type);
    const Uint64 steam_handle = SDL_GetGamepadSteamHandle(slot.gamepad.get());
    SDL_Log("Gamepad %u %s: '%s', type=%s, Steam handle=%" SDL_PRIu64 ", rumble=%s.",
            slot.instance_id, reason, name != nullptr ? name : "unknown",
            type_name != nullptr ? type_name : "unknown", steam_handle,
            slot.supports_rumble ? "yes" : "no");
}

} // namespace hover::platform
