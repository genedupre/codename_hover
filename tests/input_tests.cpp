#include "input/player_input.hpp"
#include "platform/sdl_input.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

constexpr float tolerance = 0.0001F;
int failure_count = 0;

void check(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failure_count;
    }
}

bool nearly_equal(float left, float right) { return std::abs(left - right) <= tolerance; }

void test_axis_normalization() {
    check(nearly_equal(hover::input::normalize_signed_axis({5000, 6000}), 0.0F),
          "stick values inside the dead zone become zero");
    check(nearly_equal(
              hover::input::normalize_signed_axis({std::numeric_limits<std::int16_t>::max(), 6000}),
              1.0F),
          "positive stick maximum becomes one");
    check(nearly_equal(
              hover::input::normalize_signed_axis({std::numeric_limits<std::int16_t>::min(), 6000}),
              -1.0F),
          "negative stick maximum becomes negative one");
    check(nearly_equal(hover::input::normalize_trigger({-1, 1800}), 0.0F),
          "negative trigger input is released");
    check(nearly_equal(
              hover::input::normalize_trigger({std::numeric_limits<std::int16_t>::max(), 1800}),
              1.0F),
          "trigger maximum becomes one");
}

void test_simultaneous_merge() {
    const hover::input::PlayerInput keyboard{
        .steering = -1.0F,
        .throttle = 1.0F,
        .drift_left = true,
    };
    const hover::input::PlayerInput controller{
        .steering = 0.65F,
        .brake = 0.75F,
        .drift_right = true,
        .boost = true,
    };

    const hover::input::PlayerInput merged = hover::input::merge(keyboard, controller);
    check(nearly_equal(merged.steering, -1.0F),
          "steering uses the contribution with greatest magnitude without summing");
    check(nearly_equal(merged.throttle, 1.0F) && nearly_equal(merged.brake, 0.75F),
          "independent throttle and brake actions survive simultaneous sources");
    check(merged.drift_left && merged.drift_right && merged.boost,
          "independent digital actions combine with logical OR");
}

void test_controller_exit_binding() {
    hover::platform::SdlInput input;

    SDL_Event select_button{};
    select_button.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
    select_button.gbutton.button = SDL_GAMEPAD_BUTTON_BACK;
    check(input.handle_event(select_button), "Select/Back/View requests exit in the prototype");

    SDL_Event start_button{};
    start_button.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
    start_button.gbutton.button = SDL_GAMEPAD_BUTTON_START;
    check(!input.handle_event(start_button), "Start/Menu remains available for gameplay");

    SDL_Event east_button{};
    east_button.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
    east_button.gbutton.button = SDL_GAMEPAD_BUTTON_EAST;
    check(!input.handle_event(east_button), "B/East braking does not request exit");
}

void test_virtual_controller_brake_and_boost_bindings() {
    check(SDL_InitSubSystem(SDL_INIT_GAMEPAD), "SDL gamepad subsystem initializes for input test");

    SDL_VirtualJoystickDesc description;
    SDL_INIT_INTERFACE(&description);
    description.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    description.naxes = SDL_GAMEPAD_AXIS_COUNT;
    description.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    description.name = "Codename Hover virtual test gamepad";

    const SDL_JoystickID gamepad_id = SDL_AttachVirtualJoystick(&description);
    check(gamepad_id != 0, "virtual gamepad attaches for mapping test");
    if (gamepad_id != 0) {
        {
            hover::platform::SdlInput input;
            check(input.initialize(), "input system initializes with virtual gamepad");

            SDL_Joystick* joystick = SDL_GetJoystickFromID(gamepad_id);
            check(joystick != nullptr, "attached virtual gamepad has an open joystick");
            if (joystick != nullptr) {
                check(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_EAST, true),
                      "virtual B/East button can be pressed");
                SDL_UpdateJoysticks();

                const hover::input::PlayerInput sample = input.sample_player_one();
                check(nearly_equal(sample.brake, 1.0F),
                      "B/East contributes full braking to the analog brake action");
                check(!sample.boost, "B/East braking does not activate boost");

                check(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_EAST, false),
                      "virtual B/East button can be released");
                check(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_WEST, true),
                      "virtual X/West button can be pressed");
                SDL_UpdateJoysticks();

                const hover::input::PlayerInput boost_sample = input.sample_player_one();
                check(boost_sample.boost, "X/West activates the digital boost action");
                check(nearly_equal(boost_sample.brake, 0.0F),
                      "X/West boost no longer contributes braking");

                check(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_WEST, false),
                      "virtual X/West button can be released");
                check(
                    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true),
                    "virtual LB/L1 button can be pressed");
                SDL_UpdateJoysticks();

                const hover::input::PlayerInput left_drift_sample = input.sample_player_one();
                check(left_drift_sample.drift_left && !left_drift_sample.drift_right,
                      "LB/L1 activates only the left-drift action");

                check(
                    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, true),
                    "virtual RB/R1 button can be pressed independently");
                SDL_UpdateJoysticks();

                const hover::input::PlayerInput both_drift_sample = input.sample_player_one();
                check(both_drift_sample.drift_left && both_drift_sample.drift_right,
                      "both shoulder drift actions survive sampling independently");
            }
        }
        check(SDL_DetachVirtualJoystick(gamepad_id), "virtual gamepad detaches after mapping test");
    }

    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

} // namespace

int main() {
    test_axis_normalization();
    test_simultaneous_merge();
    test_controller_exit_binding();
    test_virtual_controller_brake_and_boost_bindings();

    if (failure_count != 0) {
        std::cerr << failure_count << " input test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All input tests passed\n";
    return EXIT_SUCCESS;
}
