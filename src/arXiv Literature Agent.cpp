#include "arXiv Literature Agent.h"
#include "agent.h"
#include "config.h"
#include <nlohmann/json.hpp>
#include <string>

static std::string get_string(const nlohmann::json& j, const char* k, const std::string& def = "") {
    if (!j.contains(k) || j[k].is_null()) return def;
    return j[k].get<std::string>();
}

static int get_int(const nlohmann::json& j, const char* k, int def) {
    if (!j.contains(k) || j[k].is_null()) return def;
    return j[k].get<int>();
}

nlohmann::json run_lit_scout(const nlohmann::json& input) {
    Config cfg;
    cfg.query = get_string(input, "query");
    cfg.out_dir = get_string(input, "out_dir", "out");
    cfg.days = get_int(input, "days", 7);
    cfg.max_results = get_int(input, "max_results", 50);
    cfg.workers = get_int(input, "workers", 8);

    nlohmann::json out;
    out["query"] = cfg.query;
    out["out_dir"] = cfg.out_dir;
    out["days"] = cfg.days;
    out["max_results"] = cfg.max_results;
    out["workers"] = cfg.workers;

    if (cfg.query.empty()) {
        out["status"] = "error";
        out["error"] = "missing required field: query";
        return out;
    }

    Agent agent;
    int rc = agent.run(cfg);

    out["status"] = (rc == 0) ? "ok" : "error";
    out["return_code"] = rc;
    return out;
}