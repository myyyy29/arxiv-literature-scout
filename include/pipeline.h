#pragma once
#include <vector>
#include "config.h"
#include "models.h"

struct PipelineResult {
    std::vector<Paper> papers;
    std::vector<PaperSummary> summaries;
};

class Pipeline {
public:
    PipelineResult run(const Config& cfg);

    std::vector<Paper> filter_and_dedup(const Config& cfg, const std::vector<Paper>& papers);
    std::vector<PaperSummary> summarize_and_rank(const Config& cfg, const std::vector<Paper>& papers);
};