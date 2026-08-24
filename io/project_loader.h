#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <utility>

namespace deepiri {

struct ProjectSchematic {
    std::string name;
    std::string filepath;
};

struct ProjectSimulation {
    std::string name;
    std::string type;
    std::map<std::string, std::string> settings;
};

struct ProjectLayout {
    std::string name;
    std::string filepath;
};

struct SchematicComponentData {
    int type = 0;
    std::string label;
    double x = 0.0;
    double y = 0.0;
    std::map<std::string, std::string> properties;
};

struct SchematicWireData {
    std::vector<std::pair<double, double>> points;
};

struct Project {
    std::string name;
    std::string version = "1.0";
    std::string author;
    std::string format = "egottol-project";
    std::vector<ProjectSchematic> schematics;
    std::vector<ProjectSimulation> simulations;
    std::vector<ProjectLayout> layouts;
    std::map<std::string, std::string> metadata;
    // Embedded schematic (single-file .egt projects).
    std::vector<SchematicComponentData> components;
    std::vector<SchematicWireData> wires;
};

class ProjectLoader {
public:
    ProjectLoader();
    ~ProjectLoader();

    bool load(const std::string& project_file);
    bool save(const std::string& project_file);
    bool loadFromString(const std::string& content);
    std::string toJSON() const;

    Project getProject() const;
    void setProject(const Project& project);

    std::vector<std::string> getAvailableTemplates();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}
