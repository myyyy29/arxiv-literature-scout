#pragma once
#include <string>
#include <vector>
#include "config.h"
#include "models.h"
#include <Windows.h>   
#include <winsvc.h>    

class Reporter {
public:
 
    void write_outputs(const Config& cfg,
        const std::vector<Paper>& papers,
        const std::vector<PaperSummary>& summaries);
};