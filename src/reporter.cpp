#include "reporter.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <winsvc.h>

namespace fs = std::filesystem;

static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);

    for (unsigned char c : s) {
        switch (c) {
        case '\"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20) {
                // control chars -> \u00XX
                std::ostringstream oss;
                oss << "\\u"
                    << std::hex << std::setw(4) << std::setfill('0')
                    << (int)c;
                out += oss.str();
            }
            else {
                out.push_back((char)c);
            }
        }
    }
    return out;
}

static std::string join_as_json_array(const std::vector<std::string>& items) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) oss << ",";
        oss << "\"" << json_escape(items[i]) << "\"";
    }
    oss << "]";
    return oss.str();
}

void Reporter::write_outputs(const Config& cfg,
    const std::vector<Paper>& papers,
    const std::vector<PaperSummary>& summaries) {

    // 1) 确保输出目录存在
    fs::path out_dir(cfg.out_dir);
    std::error_code ec;
    fs::create_directories(out_dir, ec);
    if (ec) {
        std::cerr << "[Reporter] Error: cannot create out_dir: " << cfg.out_dir
            << " (" << ec.message() << ")\n";
        return;
    }

    // 2) 写 JSONL：papers.jsonl
    //    每行一个 paper，便于后续再处理/过滤/送模型
    {
        fs::path jsonl_path = out_dir / "papers.jsonl";
        std::ofstream ofs(jsonl_path, std::ios::binary);
        if (!ofs) {
            std::cerr << "[Reporter] Error: cannot open " << jsonl_path.string() << "\n";
            return;
        }

        for (const auto& p : papers) {
            // 注意：Paper 里可能字段不全，你就按你 models.h 里有的字段写
            ofs << "{"
                << "\"arxiv_id\":\"" << json_escape(p.arxiv_id) << "\","
                << "\"title\":\"" << json_escape(p.title) << "\","
                << "\"authors\":" << join_as_json_array(p.authors) << ","
                << "\"published\":\"" << json_escape(p.published) << "\","
                << "\"url\":\"" << json_escape(p.url) << "\","
                << "\"categories\":" << join_as_json_array(p.categories) << ","
                << "\"abstract\":\"" << json_escape(p.abstract_text) << "\""
                << "}\n";
        }
    }

    // 3) 为了写 report.md：把 summaries 按 arxiv_id 建索引（方便匹配）
    std::unordered_map<std::string, PaperSummary> summary_by_id;
    summary_by_id.reserve(summaries.size());
    for (const auto& s : summaries) {
        summary_by_id[s.arxiv_id] = s;
    }

    // 4) 写 Markdown：report.md
    {
        fs::path md_path = out_dir / "report.md";
        std::ofstream ofs(md_path, std::ios::binary);
        if (!ofs) {
            std::cerr << "[Reporter] Error: cannot open " << md_path.string() << "\n";
            return;
        }

        ofs << "# Literature Scout Report\n\n";
        ofs << "- Query: `" << cfg.query << "`\n";
        ofs << "- Days: " << cfg.days << "\n";
        ofs << "- Max results: " << cfg.max_results << "\n";
        ofs << "- Papers fetched: " << papers.size() << "\n\n";

        ofs << "## Results\n\n";

        for (size_t i = 0; i < papers.size(); ++i) {
            const auto& p = papers[i];

            ofs << "### " << (i + 1) << ". " << p.title << "\n\n";
            ofs << "- arXiv ID: `" << p.arxiv_id << "`\n";
            if (!p.published.empty()) ofs << "- Published: " << p.published << "\n";
            if (!p.url.empty()) ofs << "- URL: " << p.url << "\n";

            if (!p.authors.empty()) {
                ofs << "- Authors: ";
                for (size_t k = 0; k < p.authors.size(); ++k) {
                    if (k) ofs << ", ";
                    ofs << p.authors[k];
                }
                ofs << "\n";
            }

            if (!p.categories.empty()) {
                ofs << "- Categories: ";
                for (size_t k = 0; k < p.categories.size(); ++k) {
                    if (k) ofs << ", ";
                    ofs << p.categories[k];
                }
                ofs << "\n";
            }

            ofs << "\n**Abstract (preview)**\n\n";
            if (p.abstract_text.size() <= 600) {
                ofs << p.abstract_text << "\n\n";
            }
            else {
                ofs << p.abstract_text.substr(0, 600) << "...\n\n";
            }

            // 如果 summaries 里有内容（比如 score/summary），就附上
            auto it = summary_by_id.find(p.arxiv_id);
            if (it != summary_by_id.end()) {
                const auto& s = it->second;
                if (!s.structured_summary.empty()) {
                    ofs << "**Summary**\n\n" << s.structured_summary << "\n\n";
                }
                // 你如果 PaperSummary 里有 score 字段，就输出
                // ofs << "**Score**: " << s.score << "\n\n";
            }
     
            ofs << "---\n\n";
        }
    }

    std::cout << "[Reporter] wrote: " << (out_dir / "papers.jsonl").string() << "\n";
    std::cout << "[Reporter] wrote: " << (out_dir / "report.md").string() << "\n";
}