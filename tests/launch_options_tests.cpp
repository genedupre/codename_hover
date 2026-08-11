#include "core/launch_options.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

int failure_count = 0;

void check(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failure_count;
    }
}

void test_default_and_named_runway() {
    const std::span<const hover::core::DevelopmentScenarioInfo> scenarios =
        hover::core::development_scenarios();
    check(scenarios.size() == 2 && scenarios[0].name == "runway" && scenarios[1].name == "oval",
          "the scenario registry exposes runway and oval in stable order");

    const hover::core::LaunchOptionsParseResult defaults = hover::core::parse_launch_options({});
    check(defaults.succeeded(), "empty arguments parse successfully");
    check(defaults.options.scenario == hover::core::DevelopmentScenario::runway,
          "runway is the current default scenario");

    constexpr std::array named_arguments{std::string_view{"--scenario"},
                                         std::string_view{"runway"}};
    const hover::core::LaunchOptionsParseResult named =
        hover::core::parse_launch_options(named_arguments);
    check(named.succeeded() && named.options.scenario == hover::core::DevelopmentScenario::runway,
          "runway can be selected by a separate argument");

    constexpr std::array equals_arguments{std::string_view{"--scenario=runway"}};
    const hover::core::LaunchOptionsParseResult equals =
        hover::core::parse_launch_options(equals_arguments);
    check(equals.succeeded() && equals.options.scenario == hover::core::DevelopmentScenario::runway,
          "runway can be selected with equals syntax");
}

void test_named_oval() {
    constexpr std::array arguments{std::string_view{"--scenario=oval"}};
    const hover::core::LaunchOptionsParseResult result =
        hover::core::parse_launch_options(arguments);
    check(result.succeeded() && result.options.scenario == hover::core::DevelopmentScenario::oval,
          "oval can be selected by name");
    check(hover::core::scenario_name(hover::core::DevelopmentScenario::oval) == "oval",
          "oval has a stable display name");
}

void test_information_actions() {
    constexpr std::array arguments{std::string_view{"--help"},
                                   std::string_view{"--list-scenarios"}};
    const hover::core::LaunchOptionsParseResult result =
        hover::core::parse_launch_options(arguments);
    check(result.succeeded(), "information arguments parse successfully");
    check(result.options.show_help && result.options.list_scenarios,
          "help and scenario listing are retained");
}

void test_invalid_arguments() {
    constexpr std::array missing_name{std::string_view{"--scenario"}};
    check(!hover::core::parse_launch_options(missing_name).succeeded(),
          "missing scenario name is rejected");

    constexpr std::array unknown_scenario{std::string_view{"--scenario"},
                                          std::string_view{"missing"}};
    check(!hover::core::parse_launch_options(unknown_scenario).succeeded(),
          "unknown scenario is rejected");

    constexpr std::array duplicate_scenario{std::string_view{"--scenario=runway"},
                                            std::string_view{"--scenario=runway"}};
    check(!hover::core::parse_launch_options(duplicate_scenario).succeeded(),
          "duplicate scenario selection is rejected");

    constexpr std::array unknown_argument{std::string_view{"--stage"}};
    check(!hover::core::parse_launch_options(unknown_argument).succeeded(),
          "unknown arguments are rejected");
}

} // namespace

int main() {
    test_default_and_named_runway();
    test_named_oval();
    test_information_actions();
    test_invalid_arguments();

    if (failure_count != 0) {
        std::cerr << failure_count << " launch-options test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All launch-options tests passed\n";
    return EXIT_SUCCESS;
}
