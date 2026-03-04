#include "agent.h"
#include <iostream>

int Agent::run(const Config& cfg) {
    std::cout << "[Agent] Starting...\n";

    auto result = pipeline_.run(cfg);

    reporter_.write_outputs(cfg, result.papers, result.summaries);

    std::cout << "[Agent] Done.\n";
    return 0;
}