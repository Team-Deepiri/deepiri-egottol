#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace deepiri {

class PropertyEditor {
public:
  PropertyEditor();
  ~PropertyEditor();

  void setObject(void *obj);
  void *getObject() const;

  void addProperty(const std::string &name, const std::string &type);
  void setPropertyValue(const std::string &name, const std::string &value);
  std::string getPropertyValue(const std::string &name) const;

  std::vector<std::string> getPropertyNames() const;

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace deepiri