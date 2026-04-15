#include "config_manager.h"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace deepiri {

class ConfigManager::Impl {
public:
    std::map<std::string, ConfigSection> sections_;
    std::string config_file_;

    Impl() {}

    std::string configValueToString(const ConfigValue& value) {
        return std::visit([](auto&& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
                return std::to_string(arg);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            } else if constexpr (std::is_same_v<T, bool>) {
                return arg ? "true" : "false";
            } else if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<double>>) {
                std::ostringstream oss;
                for (size_t i = 0; i < arg.size(); ++i) {
                    if (i > 0) oss << ",";
                    oss << arg[i];
                }
                return oss.str();
            }
            return "";
        }, value);
    }

    bool parseLine(const std::string& line, std::string& section, std::string& key, std::string& value) {
        std::string trimmed = line;
        trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());

        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            return false;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            section = trimmed.substr(1, trimmed.length() - 2);
            return true;
        }

        size_t eq_pos = trimmed.find('=');
        if (eq_pos != std::string::npos) {
            key = trimmed.substr(0, eq_pos);
            value = trimmed.substr(eq_pos + 1);
            return true;
        }

        return false;
    }
};

ConfigManager::ConfigManager() : pImpl(std::make_unique<Impl>()) {}

ConfigManager::ConfigManager(const std::string& config_file) : pImpl(std::make_unique<Impl>()) {
    loadFromFile(config_file);
}

ConfigManager::~ConfigManager() = default;

bool ConfigManager::loadFromFile(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        return false;
    }

    pImpl->config_file_ = config_file;
    std::string current_section;
    std::string line;

    while (std::getline(file, line)) {
        std::string key, value;
        if (pImpl->parseLine(line, current_section, key, value)) {
            if (!current_section.empty() && key.empty()) {
                pImpl->sections_[current_section] = ConfigSection();
            } else if (!key.empty()) {
                pImpl->sections_[current_section].values[key] = value;
            }
        }
    }

    return true;
}

bool ConfigManager::saveToFile(const std::string& config_file) const {
    std::ofstream file(config_file);
    if (!file.is_open()) {
        return false;
    }

    file << toString();
    return true;
}

void ConfigManager::setValue(const std::string& section, const std::string& key, const ConfigValue& value) {
    pImpl->sections_[section].values[key] = value;
}

std::optional<ConfigValue> ConfigManager::getValue(const std::string& section, const std::string& key) const {
    auto section_it = pImpl->sections_.find(section);
    if (section_it == pImpl->sections_.end()) {
        return std::nullopt;
    }

    auto key_it = section_it->second.values.find(key);
    if (key_it == section_it->second.values.end()) {
        return std::nullopt;
    }

    return key_it->second;
}

bool ConfigManager::hasSection(const std::string& section) const {
    return pImpl->sections_.find(section) != pImpl->sections_.end();
}

bool ConfigManager::hasKey(const std::string& section, const std::string& key) const {
    auto section_it = pImpl->sections_.find(section);
    if (section_it == pImpl->sections_.end()) {
        return false;
    }
    return section_it->second.values.find(key) != section_it->second.values.end();
}

std::vector<std::string> ConfigManager::getSections() const {
    std::vector<std::string> sections;
    for (const auto& section : pImpl->sections_) {
        sections.push_back(section.first);
    }
    return sections;
}

std::vector<std::string> ConfigManager::getKeys(const std::string& section) const {
    std::vector<std::string> keys;
    auto section_it = pImpl->sections_.find(section);
    if (section_it != pImpl->sections_.end()) {
        for (const auto& key : section_it->second.values) {
            keys.push_back(key.first);
        }
    }
    return keys;
}

void ConfigManager::merge(const ConfigManager& other) {
    for (const auto& section : other.pImpl->sections_) {
        for (const auto& value : section.second.values) {
            setValue(section.first, value.first, value.second);
        }
    }
}

void ConfigManager::clear() {
    pImpl->sections_.clear();
}

std::string ConfigManager::toString() const {
    std::ostringstream oss;
    for (const auto& section : pImpl->sections_) {
        oss << "[" << section.first << "]\n";
        for (const auto& value : section.second.values) {
            oss << value.first << "=" << pImpl->configValueToString(value.second) << "\n";
        }
        oss << "\n";
    }
    return oss.str();
}

}