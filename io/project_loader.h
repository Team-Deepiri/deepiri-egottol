#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

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

struct Project {
  std::string name;
  std::string version;
  std::string author;
  std::vector<ProjectSchematic> schematics;
  std::vector<ProjectSimulation> simulations;
  std::vector<ProjectLayout> layouts;
  std::map<std::string, std::string> metadata;
};

class ProjectLoader {
public:
  ProjectLoader();
  ~ProjectLoader();

  bool load(const std::string &project_file);
  bool save(const std::string &project_file);

  Project getProject() const;
  void setProject(const Project &project);

  std::vector<std::string> getAvailableTemplates();

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace deepiri