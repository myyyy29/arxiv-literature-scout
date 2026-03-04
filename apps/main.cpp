#include <iostream>
#include <nlohmann/json.hpp>
#include "lit_scout_tool.h"

int main() {
    LitScoutTool tool;

    nlohmann::json input = {
        {"query", "multimodal agent"},
        {"days", 7},
        {"max_results", 10},
        {"workers", 8},
        {"out_dir", "./output"}
    };

    auto out = tool.run(input);
    std::cout << out.dump(2) << "\n";
    return 0;
}