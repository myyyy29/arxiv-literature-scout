#include "lit_scout_tool.h"
#include "arXiv Literature Agent.h"
nlohmann::json LitScoutTool::run(const nlohmann::json& input) {
    return run_lit_scout(input);
}