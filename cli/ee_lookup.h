#pragma once

#include <string>
#include <vector>

namespace deepiri {

struct EeHit {
    const char* id;
    const char* combination;
    const char* behavior;
    const char* use;
};

// Compact EE design index for headless CLI (mirrors egottol/knowledge/ee_symptoms.json).
std::vector<EeHit> lookupEeDesign(const std::string& query, int limit = 5);
std::string formatEeLookup(const std::string& query, int limit = 5);

}
