#pragma once
#include <string>
#include <vector>
struct Paper {
    std::string arxiv_id;                 //  2502.12345
    std::string title;
    std::vector<std::string> authors;
    std::string abstract_text;
    std::string published;                
    std::string url;                      
    std::vector<std::string> categories;  
};

struct PaperSummary {
    std::string arxiv_id;
    std::string structured_summary; 
    double score = 0.0;             
    std::string group;              
};