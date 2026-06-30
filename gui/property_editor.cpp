#include "property_editor.h"

namespace deepiri {

class PropertyEditor::Impl {
public:
  void *object = nullptr;
  std::map<std::string, std::string> propertyTypes;
  std::map<std::string, std::string> propertyValues;
};

PropertyEditor::PropertyEditor() : pImpl(std::make_unique<Impl>()) {}
PropertyEditor::~PropertyEditor() = default;

void PropertyEditor::setObject(void *obj) { pImpl->object = obj; }
void *PropertyEditor::getObject() const { return pImpl->object; }

void PropertyEditor::addProperty(const std::string &name,
                                 const std::string &type) {
  pImpl->propertyTypes[name] = type;
}

void PropertyEditor::setPropertyValue(const std::string &name,
                                      const std::string &value) {
  pImpl->propertyValues[name] = value;
}

std::string PropertyEditor::getPropertyValue(const std::string &name) const {
  auto it = pImpl->propertyValues.find(name);
  if (it != pImpl->propertyValues.end())
    return it->second;
  return "";
}

std::vector<std::string> PropertyEditor::getPropertyNames() const {
  std::vector<std::string> names;
  for (const auto &p : pImpl->propertyTypes)
    names.push_back(p.first);
  return names;
}

} // namespace deepiri