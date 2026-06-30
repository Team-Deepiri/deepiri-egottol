#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace deepiri {

struct SymbolPin {
  std::string name;
  int number;
  std::string direction;
  std::string electrical_type;
};

enum class SymbolType { Component, Port, Net, Text, Graphic };

struct SymbolDefinition {
  std::string name;
  std::string library;
  SymbolType type;
  std::string footprint;
  std::string description;
  std::vector<SymbolPin> pins;
  std::map<std::string, std::string> properties;
};

class SymbolLibrary {
public:
  SymbolLibrary();
  explicit SymbolLibrary(const std::string &library_path);
  ~SymbolLibrary();

  bool load(const std::string &library_path);
  bool save(const std::string &library_path);

  void addSymbol(const SymbolDefinition &symbol);
  void removeSymbol(const std::string &name);

  std::optional<SymbolDefinition> getSymbol(const std::string &name) const;
  std::vector<std::string> listSymbols() const;
  std::vector<std::string> listLibraries() const;

  bool hasSymbol(const std::string &name) const;

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace deepiri