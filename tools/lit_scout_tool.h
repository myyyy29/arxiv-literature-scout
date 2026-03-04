#pragma once
#include <nlohmann/json.hpp>

class LitScoutTool {
public:
    nlohmann::json run(const nlohmann::json& input);
};