#include "project_loader.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace deepiri {

class ProjectLoader::Impl {
public:
  Project project_;

  std::string detectFormat(const std::string &content) {
    if (content.find("<?xml") != std::string::npos) {
      return "xml";
    } else if (content.find("{") != std::string::npos &&
               content.find("}") != std::string::npos) {
      return "json";
    }
    return "json";
  }

  bool parseJSON(const std::string &content) {
    project_ = Project();
    project_.name = "Untitled";
    project_.version = "1.0";
    return true;
  }

  bool parseXML(const std::string &content) {
    project_ = Project();
    project_.name = "Untitled";
    project_.version = "1.0";
    return true;
  }

  std::string toJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"name\": \"" << project_.name << "\",\n";
    oss << "  \"version\": \"" << project_.version << "\",\n";
    oss << "  \"author\": \"" << project_.author << "\",\n";
    oss << "  \"schematics\": [\n";
    for (size_t i = 0; i < project_.schematics.size(); ++i) {
      oss << "    {\"name\": \"" << project_.schematics[i].name
          << "\", \"filepath\": \"" << project_.schematics[i].filepath << "\"}";
      if (i < project_.schematics.size() - 1)
        oss << ",";
      oss << "\n";
    }
    oss << "  ]\n";
    oss << "}";
    return oss.str();
  }
};

ProjectLoader::ProjectLoader() : pImpl(std::make_unique<Impl>()) {}
ProjectLoader::~ProjectLoader() = default;

bool ProjectLoader::load(const std::string &project_file) {
  std::ifstream file(project_file);
  if (!file.is_open()) {
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  std::string format = pImpl->detectFormat(content);

  if (format == "json") {
    return pImpl->parseJSON(content);
  } else if (format == "xml") {
    return pImpl->parseXML(content);
  }

  return false;
}

bool ProjectLoader::save(const std::string &project_file) {
  std::ofstream file(project_file);
  if (!file.is_open()) {
    return false;
  }

  file << pImpl->toJSON();
  return true;
}

Project ProjectLoader::getProject() const { return pImpl->project_; }

void ProjectLoader::setProject(const Project &project) {
  pImpl->project_ = project;
}

std::vector<std::string> ProjectLoader::getAvailableTemplates() {
  return {"analog", "digital", "mixed-signal", "rf", "power"};
}

} // namespace deepiri