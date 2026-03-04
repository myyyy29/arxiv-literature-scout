#pragma once
#include <string>
struct Config {
    //default
    std::string query;              // query
    int days = 7;                   
    int max_results = 50;           
    int workers = 8;                
    std::string out_dir = "./output";
};