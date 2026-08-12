#include "core/launch_options.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace hover::core {
namespace {

constexpr std::array scenario_registry{
    DevelopmentScenarioInfo{
        DevelopmentScenario::runway,
        "runway",
        "Generated ship, long presentation runway, and free planar driving.",
    },
    DevelopmentScenarioInfo{
        DevelopmentScenario::oval,
        "oval",
        "Flat closed sampled oval with deterministic track-attached driving.",
    },
    DevelopmentScenarioInfo{
        DevelopmentScenario::speedway,
        "speedway",
        "First map prototype: attached driving on an oval speedway with banked turns.",
    },
    DevelopmentScenarioInfo{
        DevelopmentScenario::speedway_physics,
        "speedway_physics",
        "Banked speedway using world-space momentum, grip, and directional drift.",
    },
};

std::optional<DevelopmentScenario> parse_scenario(std::string_view name) {
    for (const DevelopmentScenarioInfo& scenario : scenario_registry) {
        if (name == scenario.name) {
            return scenario.id;
        }
    }
    return std::nullopt;
}

} // namespace

LaunchOptionsParseResult parse_launch_options(std::span<const std::string_view> arguments) {
    LaunchOptionsParseResult result{};
    bool scenario_was_set = false;

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string_view argument = arguments[index];
        if (argument == "--help" || argument == "-h") {
            result.options.show_help = true;
            continue;
        }
        if (argument == "--list-scenarios") {
            result.options.list_scenarios = true;
            continue;
        }

        std::string_view scenario_value;
        if (argument == "--scenario") {
            if (index + 1 >= arguments.size()) {
                result.error = "--scenario requires a scenario name";
                return result;
            }
            scenario_value = arguments[++index];
        } else if (argument.starts_with("--scenario=")) {
            scenario_value = argument.substr(std::string_view{"--scenario="}.size());
        } else {
            result.error = "unknown argument: " + std::string{argument};
            return result;
        }

        if (scenario_was_set) {
            result.error = "--scenario may only be specified once";
            return result;
        }
        scenario_was_set = true;

        const std::optional<DevelopmentScenario> scenario = parse_scenario(scenario_value);
        if (!scenario.has_value()) {
            result.error = "unknown scenario: " + std::string{scenario_value};
            return result;
        }
        result.options.scenario = *scenario;
    }

    return result;
}

std::span<const DevelopmentScenarioInfo> development_scenarios() { return scenario_registry; }

std::string_view scenario_name(DevelopmentScenario scenario) {
    for (const DevelopmentScenarioInfo& info : scenario_registry) {
        if (scenario == info.id) {
            return info.name;
        }
    }
    return "unknown";
}

} // namespace hover::core
