#include "ee_lookup.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace deepiri {

namespace {

struct Entry {
    const char* id;
    const char* symptoms;  // space-separated keywords
    const char* combination;
    const char* behavior;
    const char* use;
};

#include "ee_lookup_entries.inc"

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

}  // namespace

std::vector<EeHit> lookupEeDesign(const std::string& query, int limit) {
    std::string q = lower(query);
    struct Scored { int score; EeHit hit; };
    std::vector<Scored> scored;
    for (size_t i = 0; i < kEntryCount; ++i) {
        const auto& e = kEntries[i];
        std::string bag = lower(std::string(e.symptoms) + " " + e.combination + " " + e.use);
        int score = 0;
        std::istringstream iss(q);
        std::string tok;
        while (iss >> tok) {
            if (tok.size() < 2) continue;
            if (bag.find(tok) != std::string::npos) score += (tok.size() > 3 ? 3 : 2);
        }
        if (score > 0) {
            scored.push_back({score, EeHit{e.id, e.combination, e.behavior, e.use}});
        }
    }
    std::sort(scored.begin(), scored.end(),
              [](const Scored& a, const Scored& b) { return a.score > b.score; });
    std::vector<EeHit> out;
    for (size_t i = 0; i < scored.size() && static_cast<int>(i) < limit; ++i) {
        out.push_back(scored[i].hit);
    }
    return out;
}

std::string formatEeLookup(const std::string& query, int limit) {
    auto hits = lookupEeDesign(query, limit);
    std::ostringstream oss;
    if (hits.empty()) {
        oss << "No EE hits for '" << query << "'. Try: flyback, LED, buck, crystal, floorplan\n"
            << "Full docs: docs/ee/\n";
        return oss.str();
    }
    oss << "EE design matches for '" << query << "':\n";
    for (const auto& h : hits) {
        oss << "  [" << h.id << "] " << h.combination << "\n"
            << "      " << h.behavior << "\n"
            << "      Use: " << h.use << "\n";
    }
    oss << "Docs: docs/ee/  |  Python: egottol.knowledge.lookup_ee_design\n";
    return oss.str();
}

}
