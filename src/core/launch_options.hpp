#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace hover::core {

enum class DevelopmentScenario : std::uint8_t {
    runway,
    oval,
    speedway,
    speedway_physics,
};

struct LaunchOptions {
    DevelopmentScenario scenario = DevelopmentScenario::runway;
    bool show_help = false;
    bool list_scenarios = false;
};

struct DevelopmentScenarioInfo {
    DevelopmentScenario id;
    std::string_view name;
    std::string_view description;
};

struct LaunchOptionsParseResult {
    LaunchOptions options;
    std::string error;

    [[nodiscard]] bool succeeded() const { return error.empty(); }
};

[[nodiscard]] LaunchOptionsParseResult
parse_launch_options(std::span<const std::string_view> arguments);
[[nodiscard]] std::span<const DevelopmentScenarioInfo> development_scenarios();
[[nodiscard]] std::string_view scenario_name(DevelopmentScenario scenario);

} // namespace hover::core
