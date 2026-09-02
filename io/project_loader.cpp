#include "project_loader.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace deepiri {

namespace {

std::string escapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

// Minimal JSON helpers — enough for our project format, no external deps.
void skipWs(const std::string& s, size_t& i) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
}

bool match(const std::string& s, size_t& i, char c) {
    skipWs(s, i);
    if (i < s.size() && s[i] == c) {
        ++i;
        return true;
    }
    return false;
}

bool parseString(const std::string& s, size_t& i, std::string& out) {
    skipWs(s, i);
    if (i >= s.size() || s[i] != '"') return false;
    ++i;
    out.clear();
    while (i < s.size()) {
        char c = s[i++];
        if (c == '"') return true;
        if (c == '\\' && i < s.size()) {
            char e = s[i++];
            switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                default: out += e; break;
            }
        } else {
            out += c;
        }
    }
    return false;
}

bool parseNumber(const std::string& s, size_t& i, double& out) {
    skipWs(s, i);
    size_t start = i;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.' ||
                            s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-')) {
        // Allow exponent signs after e/E only — keep simple and use stod.
        ++i;
        if (i > start + 64) break;
    }
    // Rewind to a clean stod parse.
    i = start;
    try {
        size_t idx = 0;
        out = std::stod(s.substr(i), &idx);
        i += idx;
        return idx > 0;
    } catch (...) {
        return false;
    }
}

bool findKey(const std::string& s, size_t from, const std::string& key, size_t& valuePos) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = s.find(pattern, from);
    if (pos == std::string::npos) return false;
    pos += pattern.size();
    skipWs(s, pos);
    if (pos >= s.size() || s[pos] != ':') return false;
    ++pos;
    skipWs(s, pos);
    valuePos = pos;
    return true;
}

}  // namespace

class ProjectLoader::Impl {
public:
    Project project_;

    std::string serialize() const {
        std::ostringstream oss;
        oss << "{\n";
        oss << "  \"format\": \"" << escapeJson(project_.format) << "\",\n";
        oss << "  \"name\": \"" << escapeJson(project_.name) << "\",\n";
        oss << "  \"version\": \"" << escapeJson(project_.version) << "\",\n";
        oss << "  \"author\": \"" << escapeJson(project_.author) << "\",\n";
        oss << "  \"components\": [\n";
        for (size_t i = 0; i < project_.components.size(); ++i) {
            const auto& c = project_.components[i];
            oss << "    {\"type\": " << c.type
                << ", \"label\": \"" << escapeJson(c.label) << "\""
                << ", \"x\": " << c.x
                << ", \"y\": " << c.y
                << ", \"properties\": {";
            bool first = true;
            for (const auto& kv : c.properties) {
                if (!first) oss << ", ";
                first = false;
                oss << "\"" << escapeJson(kv.first) << "\": \"" << escapeJson(kv.second) << "\"";
            }
            oss << "}}";
            if (i + 1 < project_.components.size()) oss << ",";
            oss << "\n";
        }
        oss << "  ],\n";
        oss << "  \"wires\": [\n";
        for (size_t i = 0; i < project_.wires.size(); ++i) {
            const auto& w = project_.wires[i];
            oss << "    {\"points\": [";
            for (size_t j = 0; j < w.points.size(); ++j) {
                oss << "[" << w.points[j].first << ", " << w.points[j].second << "]";
                if (j + 1 < w.points.size()) oss << ", ";
            }
            oss << "]}";
            if (i + 1 < project_.wires.size()) oss << ",";
            oss << "\n";
        }
        oss << "  ]\n";
        oss << "}\n";
        return oss.str();
    }

    bool parse(const std::string& content) {
        project_ = Project();
        project_.format = "egottol-project";
        project_.version = "1.0";

        size_t pos = 0;
        if (findKey(content, 0, "name", pos)) {
            parseString(content, pos, project_.name);
        }
        if (findKey(content, 0, "version", pos)) {
            parseString(content, pos, project_.version);
        }
        if (findKey(content, 0, "author", pos)) {
            parseString(content, pos, project_.author);
        }
        if (findKey(content, 0, "format", pos)) {
            parseString(content, pos, project_.format);
        }

        // Components array
        size_t compKey = content.find("\"components\"");
        if (compKey != std::string::npos) {
            size_t arr = content.find('[', compKey);
            size_t arrEnd = content.find(']', arr);
            if (arr != std::string::npos && arrEnd != std::string::npos) {
                size_t i = arr + 1;
                while (i < arrEnd) {
                    size_t obj = content.find('{', i);
                    if (obj == std::string::npos || obj >= arrEnd) break;
                    size_t objEnd = content.find('}', obj);
                    if (objEnd == std::string::npos || objEnd > arrEnd) break;
                    std::string objStr = content.substr(obj, objEnd - obj + 1);

                    SchematicComponentData c;
                    size_t vp = 0;
                    if (findKey(objStr, 0, "type", vp)) {
                        double t = 0;
                        if (parseNumber(objStr, vp, t)) c.type = static_cast<int>(t);
                    }
                    if (findKey(objStr, 0, "label", vp)) {
                        parseString(objStr, vp, c.label);
                    }
                    if (findKey(objStr, 0, "x", vp)) {
                        parseNumber(objStr, vp, c.x);
                    }
                    if (findKey(objStr, 0, "y", vp)) {
                        parseNumber(objStr, vp, c.y);
                    }
                    size_t propsKey = objStr.find("\"properties\"");
                    if (propsKey != std::string::npos) {
                        size_t brace = objStr.find('{', propsKey);
                        size_t braceEnd = objStr.find('}', brace);
                        if (brace != std::string::npos && braceEnd != std::string::npos) {
                            size_t p = brace + 1;
                            while (p < braceEnd) {
                                std::string k, v;
                                if (!parseString(objStr, p, k)) break;
                                if (!match(objStr, p, ':')) break;
                                if (!parseString(objStr, p, v)) break;
                                c.properties[k] = v;
                                skipWs(objStr, p);
                                if (p < braceEnd && objStr[p] == ',') ++p;
                            }
                        }
                    }
                    project_.components.push_back(std::move(c));
                    i = objEnd + 1;
                }
            }
        }

        // Wires array
        size_t wireKey = content.find("\"wires\"");
        if (wireKey != std::string::npos) {
            size_t arr = content.find('[', wireKey);
            // Find matching end of wires array — scan for "]," or "]\n}" near end.
            // Nested arrays make naive find(']') wrong; parse objects instead.
            if (arr != std::string::npos) {
                size_t i = arr + 1;
                skipWs(content, i);
                while (i < content.size() && content[i] != ']') {
                    if (content[i] != '{') {
                        ++i;
                        continue;
                    }
                    size_t obj = i;
                    int depth = 0;
                    size_t j = obj;
                    for (; j < content.size(); ++j) {
                        if (content[j] == '{') ++depth;
                        else if (content[j] == '}') {
                            --depth;
                            if (depth == 0) {
                                ++j;
                                break;
                            }
                        }
                    }
                    std::string objStr = content.substr(obj, j - obj);
                    SchematicWireData w;
                    size_t ptsKey = objStr.find("\"points\"");
                    if (ptsKey != std::string::npos) {
                        size_t pa = objStr.find('[', ptsKey);
                        size_t p = pa + 1;
                        while (p < objStr.size() && objStr[p] != ']') {
                            skipWs(objStr, p);
                            if (objStr[p] == '[') {
                                ++p;
                                double x = 0, y = 0;
                                if (!parseNumber(objStr, p, x)) break;
                                match(objStr, p, ',');
                                if (!parseNumber(objStr, p, y)) break;
                                match(objStr, p, ']');
                                w.points.emplace_back(x, y);
                            } else {
                                ++p;
                            }
                            skipWs(objStr, p);
                            if (p < objStr.size() && objStr[p] == ',') ++p;
                        }
                    }
                    project_.wires.push_back(std::move(w));
                    i = j;
                    skipWs(content, i);
                    if (i < content.size() && content[i] == ',') ++i;
                    skipWs(content, i);
                }
            }
        }

        if (project_.name.empty()) project_.name = "Untitled";
        return true;
    }
};

ProjectLoader::ProjectLoader() : pImpl(std::make_unique<Impl>()) {}
ProjectLoader::~ProjectLoader() = default;

bool ProjectLoader::load(const std::string& project_file) {
    std::ifstream file(project_file);
    if (!file.is_open()) return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromString(buffer.str());
}

bool ProjectLoader::loadFromString(const std::string& content) {
    return pImpl->parse(content);
}

bool ProjectLoader::save(const std::string& project_file) {
    std::ofstream file(project_file);
    if (!file.is_open()) return false;
    file << toJSON();
    return true;
}

std::string ProjectLoader::toJSON() const {
    return pImpl->serialize();
}

Project ProjectLoader::getProject() const {
    return pImpl->project_;
}

void ProjectLoader::setProject(const Project& project) {
    pImpl->project_ = project;
}

std::vector<std::string> ProjectLoader::getAvailableTemplates() {
    return {"analog", "digital", "mixed-signal", "rf", "power"};
}

}
