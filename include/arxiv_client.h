#pragma once
#include <string>
#include <vector>
#include "models.h"
#include "config.h"

class ArxivClient {
public:
    std::string build_url(const Config& cfg, int start) const;
    std::string http_get(const std::string& url) const;
    std::vector<Paper> fetch(const Config& cfg) const;
};