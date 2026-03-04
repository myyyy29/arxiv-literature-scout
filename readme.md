# C++ arXiv Literature Scout (Agent-style Pipeline)

This project implements a simple C++ literature scouting system that fetches recent papers from arXiv, ranks them based on query relevance, and generates structured reports.

The system is designed in a modular way. Inspired by the idea of agent-style systems (such as agents-cpp), the core pipeline logic is separated from a JSON-based tool interface so that the system can be reused or integrated with other components in the future.

Main capabilities:

- Fetch recent papers from arXiv using HTTP (libcurl)
- Remove duplicate papers based on arXiv ID
- Compute relevance scores concurrently using `std::async`
- Select the Top 5 most relevant papers
- Generate structured output files (JSONL and Markdown)
## System Architecture

The system is organized into layered components:

Tool (JSON interface)
        ↓
      Agent
        ↓
     Pipeline
        ↓
ArxivClient → Fetch
        ↓
Filter & Dedup
        ↓
Concurrent Ranking
        ↓
Reporter (JSONL + Markdown)
## Concurrency Model

Paper summarization and scoring are executed concurrently using `std::async`.

- Configurable worker limit
- Futures are collected in batches
- Ensures bounded parallelism
## Relevance Scoring

Relevance is computed using a simple keyword-based scoring method.

- Phrase match in title: higher score
- Phrase match in abstract: medium score
- Keyword match in title: +2
- Keyword match in abstract: +1
- Extra bonus if a keyword appears in both title and abstract

Papers are then sorted by score, and the Top 5 most relevant papers are selected.

##Demo Video

Demo video: https://drive.google.com/drive/folders/19a0Mp7mQovGaZa04LofFb7Sz2VOagVic?usp=drive_link
## Output

The system generates two output files: a machine-readable JSONL file and a human-readable Markdown report.

Example JSONL output (`papers.jsonl`):

```json
{"arxiv_id":"2510.13343","title":"AOAD-MAT: Transformer-based multi-agent deep reinforcement learning model considering agents' order of action decisions","score":16,"url":"http://arxiv.org/abs/2510.13343v1"}
### 1. AOAD-MAT: Transformer-based multi-agent deep reinforcement learning model considering agents' order of action decisions

- arXiv ID: 2510.13343
- Authors: Shota Takayama, Katsuhide Fujita
- Categories: cs.AI, cs.LG, cs.MA
 Abstract (preview)
 Multi-agent reinforcement learning focuses on training the behaviors of multiple learning agents that coexist in a shared environment...
 Summary
 Title: AOAD-MAT: Transformer-based multi-agent deep reinforcement learning model considering agents' order of action decisions  
Relevance Score: 16
## Build

Requirements:
- C++17
- CMake
- vcpkg (manifest mode)

Dependencies:
- curl
- nlohmann-json

Build (Visual Studio):
1. Open folder
2. Configure preset
3. Build target `lit_app`
4. Run

Build (CLI):
cmake -S . -B out/build -G Ninja
cmake --build out/build

