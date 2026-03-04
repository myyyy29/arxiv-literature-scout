cmake_minimum_required(VERSION 3.20)
project(literature_agent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL REQUIRED)

# ---- Core Library ----
add_library(litcore STATIC
    "src/arXiv Literature Agent.cpp"
    src/arxiv_client.cpp
    src/agent.cpp
    src/pipeline.cpp
    
    src/reporter.cpp
)

target_include_directories(litcore PUBLIC
    include
)

target_link_libraries(litcore PUBLIC
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

# ---- Tool Wrapper ----
add_library(littools STATIC
    tools/lit_scout_tool.cpp
)

target_include_directories(littools PUBLIC
    tools
    include
)

target_link_libraries(littools PUBLIC
    litcore
    nlohmann_json::nlohmann_json
)

# ---- Executable ----
add_executable(lit_app
    apps/main.cpp
)

target_link_libraries(lit_app PRIVATE
    littools
)