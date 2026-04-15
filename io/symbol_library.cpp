#include "symbol_library.h"
#include <fstream>
#include <sstream>

namespace deepiri {

class SymbolLibrary::Impl {
public:
    std::map<std::string, SymbolDefinition> symbols_;
    std::string library_path_;

    void registerStandardSymbols() {
        SymbolDefinition r;
        r.name = "R";
        r.library = "Device";
        r.type = SymbolType::Component;
        r.footprint = "Resistor_SMD";
        r.description = "Resistor";
        r.pins = {};
        symbols_["R"] = r;

        SymbolDefinition c;
        c.name = "C";
        c.library = "Device";
        c.type = SymbolType::Component;
        c.footprint = "Capacitor_SMD";
        c.description = "Capacitor";
        c.pins = {};
        symbols_["C"] = c;

        SymbolDefinition l;
        l.name = "L";
        l.library = "Device";
        l.type = SymbolType::Component;
        l.footprint = "Inductor_SMD";
        l.description = "Inductor";
        l.pins = {};
        symbols_["L"] = l;
    }
};

SymbolLibrary::SymbolLibrary() : pImpl(std::make_unique<Impl>()) {
    pImpl->registerStandardSymbols();
}

SymbolLibrary::SymbolLibrary(const std::string& library_path) : pImpl(std::make_unique<Impl>()) {
    pImpl->library_path_ = library_path;
    pImpl->registerStandardSymbols();
    load(library_path);
}

SymbolLibrary::~SymbolLibrary() = default;

bool SymbolLibrary::load(const std::string& library_path) {
    pImpl->library_path_ = library_path;
    return true;
}

bool SymbolLibrary::save(const std::string& library_path) {
    pImpl->library_path_ = library_path;
    return true;
}

void SymbolLibrary::addSymbol(const SymbolDefinition& symbol) {
    pImpl->symbols_[symbol.name] = symbol;
}

void SymbolLibrary::removeSymbol(const std::string& name) {
    pImpl->symbols_.erase(name);
}

std::optional<SymbolDefinition> SymbolLibrary::getSymbol(const std::string& name) const {
    auto it = pImpl->symbols_.find(name);
    if (it != pImpl->symbols_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> SymbolLibrary::listSymbols() const {
    std::vector<std::string> names;
    for (const auto& sym : pImpl->symbols_) {
        names.push_back(sym.first);
    }
    return names;
}

std::vector<std::string> SymbolLibrary::listLibraries() const {
    std::vector<std::string> libs;
    for (const auto& sym : pImpl->symbols_) {
        libs.push_back(sym.second.library);
    }
    return libs;
}

bool SymbolLibrary::hasSymbol(const std::string& name) const {
    return pImpl->symbols_.find(name) != pImpl->symbols_.end();
}

}