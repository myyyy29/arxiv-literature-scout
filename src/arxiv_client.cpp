#include "arxiv_client.h"
#include <sstream>
#include <stdexcept>
#include <curl/curl.h>
#include <algorithm>
#include < cctype >
#include <iostream>

using namespace std;
namespace {
    struct CurlGlobalInit {
        CurlGlobalInit() { curl_global_init(CURL_GLOBAL_DEFAULT); }
        ~CurlGlobalInit() { curl_global_cleanup(); }
    };
    static CurlGlobalInit g_curl_init;
} 
static void trim_inplace(std::string& s) {
    auto is_space = [](unsigned char c) { return std::isspace(c); };
    while (!s.empty() && is_space((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && is_space((unsigned char)s.back())) s.pop_back();
}
static std::string url_encode(CURL* curl, const std::string& s) {
    char* enc = curl_easy_escape(curl, s.c_str(), (int)s.size());
    if (!enc) return s;
    std::string out(enc);
    curl_free(enc);
    return out;
}
static std::string extract_tag(const std::string& xml, const std::string& tag) {
    std::string open = "<" + tag + ">";
    std::string close = "</" + tag + ">";
    size_t a = xml.find(open);
    if (a == std::string::npos) return "";
    a += open.size();
    size_t b = xml.find(close, a);
    if (b == std::string::npos) return "";
    std::string out = xml.substr(a, b - a);
    trim_inplace(out);
    return out;
}
static std::vector<std::string> extract_all_tags(const std::string& xml, const std::string& tag) {
    std::vector<std::string> out;
    std::string open = "<" + tag + ">";
    std::string close = "</" + tag + ">";
    size_t pos = 0;

    while (true) {
        size_t a = xml.find(open, pos);
        if (a == std::string::npos) break;
        a += open.size();
        size_t b = xml.find(close, a);
        if (b == std::string::npos) break;

        std::string v = xml.substr(a, b - a);
        trim_inplace(v);
        out.push_back(v);
        pos = b + close.size();
    }
    return out;
}
static std::vector<std::string> split_entries(const std::string& feed_xml) {
    std::vector<std::string> entries;
    const std::string open = "<entry>";
    const std::string close = "</entry>";

    size_t pos = 0;
    while (true) {
        size_t a = feed_xml.find(open, pos);
        if (a == std::string::npos) break;
        size_t b = feed_xml.find(close, a);
        if (b == std::string::npos) break;

        b += close.size();
        entries.push_back(feed_xml.substr(a, b - a));
        pos = b;
    }
    return entries;
}
static std::vector<std::string> extract_all_category_terms(const std::string& entry_xml) {
    std::vector<std::string> out;
    const std::string key = "term=\"";
    size_t pos = 0;

    while (true) {
        size_t a = entry_xml.find(key, pos);
        if (a == std::string::npos) break;
        a += key.size();
        size_t b = entry_xml.find('"', a);
        if (b == std::string::npos) break;

        std::string term = entry_xml.substr(a, b - a);
        trim_inplace(term);
        out.push_back(term);
        pos = b + 1;
    }

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}
static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* s = static_cast<std::string*>(userdata);
    s->append(ptr, size * nmemb);
    return size * nmemb;
}
static std::string extract_arxiv_id_from_url(const std::string& url) {
    const std::string marker = "/abs/";
    size_t p = url.find(marker);
    if (p == std::string::npos) return "";
    p += marker.size();

    std::string id = url.substr(p);
    size_t q = id.find('?');
    if (q != std::string::npos) id = id.substr(0, q);

    size_t v = id.find('v');
    if (v != std::string::npos) id = id.substr(0, v);

    trim_inplace(id);
    return id;
}
std::vector<Paper> ArxivClient::fetch(const Config& cfg) const {
    std::vector<Paper> papers;

    std::string url = build_url(cfg, 0);
    std::cout << "[ArxivClient] url = " << url << "\n";

    std::string xml = http_get(url);
    std::cout << "[ArxivClient] http_get bytes = " << xml.size() << "\n";


    if (xml.empty()) {
        throw std::runtime_error("http_get returned empty response");
    }
    if (xml.find("<feed") == std::string::npos) {
        std::string head = xml.substr(0, std::min<size_t>(200, xml.size()));
        throw std::runtime_error("response is not an Atom <feed>. head=" + head);
    }

    auto entries = split_entries(xml);

    for (const auto& e : entries) {
        Paper p;
        p.url = extract_tag(e, "id");
        p.arxiv_id = extract_arxiv_id_from_url(p.url);
        p.title = extract_tag(e, "title");
        p.abstract_text = extract_tag(e, "summary");
        p.published = extract_tag(e, "published");
        p.authors = extract_all_tags(e, "name");
        p.categories = extract_all_category_terms(e);
        papers.push_back(std::move(p));
    }

    return papers;
}std::string ArxivClient::build_url(const Config& cfg, int start) const {
    CURL* curl = curl_easy_init();  // encode

    std::string q_enc = cfg.query;
    if (curl) {
        q_enc = url_encode(curl, cfg.query);
        curl_easy_cleanup(curl);
    }

    std::ostringstream oss;
    oss << "http://export.arxiv.org/api/query?"
        << "search_query=all:" << q_enc
        << "&start=" << start
        << "&max_results=" << cfg.max_results;

    return oss.str();
}
std::string ArxivClient::http_get(const std::string& url) const {
    std::cout << "[http_get] init\n" << std::flush;

    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::cout << "[http_get] setopt url=" << url << "\n" << std::flush;

    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

   
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "arxiv-scout/0.1");

    std::cout << "[http_get] perform...\n" << std::flush;
    CURLcode res = curl_easy_perform(curl);
    std::cout << "[http_get] perform done res=" << res << "\n" << std::flush;

    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    std::cout << "[http_get] http code=" << code << "\n" << std::flush;

    curl_easy_cleanup(curl);
    std::cout << "[http_get] cleanup done\n" << std::flush;

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl_easy_perform failed: ") + curl_easy_strerror(res));
    }
    if (code >= 400) {
        throw std::runtime_error("HTTP error code: " + std::to_string(code));
    }
    return body;
}