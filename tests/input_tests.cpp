#include "input/player_input.hpp"

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
        .drift = true,
    };
    const hover::input::PlayerInput controller{
        .steering = 0.65F,
        .brake = 0.75F,
        .boost = true,
    };

    const hover::input::PlayerInput merged = hover::input::merge(keyboard, controller);
    check(nearly_equal(merged.steering, -1.0F),
          "steering uses the contribution with greatest magnitude without summing");
    check(nearly_equal(merged.throttle, 1.0F) && nearly_equal(merged.brake, 0.75F),
          "independent throttle and brake actions survive simultaneous sources");
    check(merged.drift && merged.boost, "digital actions combine with logical OR");
}

} // namespace

int main() {
    test_axis_normalization();
    test_simultaneous_merge();

    if (failure_count != 0) {
        std::cerr << failure_count << " input test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All input tests passed\n";
    return EXIT_SUCCESS;
}
