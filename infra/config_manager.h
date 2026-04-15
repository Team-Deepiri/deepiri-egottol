#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <variant>
#include <optional>
#include <fstream>

namespace deepiri {

using ConfigValue = std::variant<int, double, std::string, bool, std::vector<int>, std::vector<double>>;

struct ConfigSection {
    std::map<std::string, ConfigValue> values;
};

class ConfigManager {
public:
    ConfigManager();
    explicit ConfigManager(const std::string& config_file);
    ~ConfigManager();

    bool loadFromFile(const std::string& config_file);
    bool saveToFile(const std::string& config_file) const;

    void setValue(const std::string& section, const std::string& key, const ConfigValue& value);
    std::optional<ConfigValue> getValue(const std::string& section, const std::string& key) const;

    bool hasSection(const std::string& section) const;
    bool hasKey(const std::string& section, const std::string& key) const;

    std::vector<std::string> getSections() const;
    std::vector<std::string> getKeys(const std::string& section) const;

    void merge(const ConfigManager& other);
    void clear();

    std::string toString() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}