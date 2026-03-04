#pragma once
#include "config.h"
#include "pipeline.h"
#include "reporter.h"

class Agent {
public:
    int run(const Config& cfg);

private:
    Pipeline pipeline_;
    Reporter reporter_;
};