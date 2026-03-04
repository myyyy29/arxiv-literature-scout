#include "pipeline.h"
#include "arxiv_client.h"
#include <unordered_set>
#include <future>     
#include <algorithm> 
#include <thread>
#include <cctype>
#include <iostream>
#include <sstream>
#include <unordered_map>
PipelineResult Pipeline::run(const Config& cfg) {
    try {
        std::cout << "[Pipeline] Fetching from arXiv...\n";
        ArxivClient client;
        auto raw = client.fetch(cfg);

        std::cout << "[Pipeline] fetched papers = " << raw.size() << "\n";

        auto cleaned = filter_and_dedup(cfg, raw);
        auto top_summaries = summarize_and_rank(cfg, cleaned);

        // 用 arxiv_id 索引
        std::unordered_map<std::string, Paper> by_id;
        by_id.reserve(cleaned.size());
        for (const auto& p : cleaned) {
            if (!p.arxiv_id.empty()) by_id[p.arxiv_id] = p;
        }

        std::vector<Paper> top_papers;
        top_papers.reserve(top_summaries.size());
        for (const auto& s : top_summaries) {
            auto it = by_id.find(s.arxiv_id);
            if (it != by_id.end()) {
                top_papers.push_back(it->second);
            }
        }

        // return Top5
        return PipelineResult{ top_papers, top_summaries };
    }
    catch (const std::exception& e) {
        std::cerr << "[Pipeline] ERROR: " << e.what() << "\n";
        return PipelineResult{};
    }
    catch (...) {
        std::cerr << "[Pipeline] ERROR: unknown exception\n";
        return PipelineResult{};
    }
}
std::vector<Paper> Pipeline::filter_and_dedup(const Config& /*cfg*/, const std::vector<Paper>& papers) {
    //  dedup by arxiv_id if available.
    std::unordered_set<std::string> seen;
    std::vector<Paper> out;
    out.reserve(papers.size());

    for (const auto& p : papers) {
        if (!p.arxiv_id.empty()) {
            if (seen.insert(p.arxiv_id).second) {
                out.push_back(p);
            }
        }
        else {
            // 如果无id，保存
            out.push_back(p);
        }
    }
    return out;
}
std::vector<PaperSummary>
Pipeline::summarize_and_rank(const Config& cfg, const std::vector<Paper>& papers) {
    std::vector<PaperSummary> summaries;
    summaries.reserve(papers.size());

    // worker >=1
    const int workers = (cfg.workers > 0) ? cfg.workers : 1;

    // 函数summarize_one
    auto summarize_one = [&cfg](const Paper& p) -> PaperSummary {
        PaperSummary s;
        s.arxiv_id = p.arxiv_id;

        // 0) score
        s.score = 0.0;

        auto to_lower = [](std::string x) {
            std::transform(x.begin(), x.end(), x.begin(),
                [](unsigned char c) { return (char)std::tolower(c); });
            return x;
            };

        auto normalize = [](std::string x) {
            // 标点/特殊符号->空格，避免 token 粘连
            for (char& c : x) {
                if (!(std::isalnum((unsigned char)c) || c == '+' || c == ' ')) c = ' ';
            }
            return x;
            };

        auto count_occurrences = [](const std::string& text, const std::string& word) -> int {
            if (word.empty()) return 0;
            int cnt = 0;
            size_t pos = 0;
            while ((pos = text.find(word, pos)) != std::string::npos) {
                cnt++;
                pos += word.size();
            }
            return cnt;
            };

        std::string title = normalize(to_lower(p.title));
        std::string abstract = normalize(to_lower(p.abstract_text));

        std::string query_raw = normalize(to_lower(cfg.query));

        // 1) 短语 bonus：整句出现
        //（对 title 权重大一些）
        if (!query_raw.empty()) {
            if (title.find(query_raw) != std::string::npos)   s.score += 8.0;
            if (abstract.find(query_raw) != std::string::npos) s.score += 4.0;
        }

        // 2) 把 + 和空格都当分隔
        for (char& c : query_raw) {
            if (c == '+') c = ' ';
        }

        std::istringstream iss(query_raw);
        std::string word;
        while (iss >> word) {
            if (word.size() < 2) continue;

            // 3) 计数出现越多分越高
            int t = count_occurrences(title, word);
            int a = count_occurrences(abstract, word);

            // title权重高：每次出现 +2；abstract 每次出现 +1
            s.score += 2.0 * t;
            s.score += 1.0 * a;

            //若title 和 abstract 都有
            if (t > 0 && a > 0) s.score += 1.0;
        }

        // 1) 再生成 structured_summary
        std::string abstract_preview = p.abstract_text.substr(0, std::min<size_t>(300, p.abstract_text.size()));

        s.structured_summary =
            "Title: " + p.title + "\n"
            "Problem/Idea: " + abstract_preview + "\n\n"
            "Method: (heuristic extraction)\n"
            "Notes: query=" + cfg.query + "\n"
            "Relevance Score: " + std::to_string(s.score);

        return s;
        };

    std::vector<std::future<PaperSummary>> futures;
    futures.reserve(workers);

    for (const auto& p : papers) {
        // 启动
        futures.push_back(std::async(std::launch::async, summarize_one, std::cref(p)));

        // 达到并发上限
        if ((int)futures.size() >= workers) {
            for (auto& f : futures) {
                summaries.push_back(f.get());
            }
            futures.clear();
        }
    }

    // 回收
    for (auto& f : futures) {
        summaries.push_back(f.get());
    }
    std::sort(summaries.begin(), summaries.end(),
        [](const PaperSummary& a, const PaperSummary& b) {
            return a.score > b.score;
        });
    const size_t K = 5;
    if (summaries.size() > K) summaries.resize(K);


     std::sort(summaries.begin(), summaries.end(),
            [](const PaperSummary& a, const PaperSummary& b) { return a.score > b.score; });

    return summaries;
}