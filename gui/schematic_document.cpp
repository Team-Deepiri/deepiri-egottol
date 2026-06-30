#include "schematic_document.h"

namespace deepiri {

SchematicDocument::SchematicDocument() = default;

void SchematicDocument::addComponent(SchematicComponent comp) {
    // TODO Stage 3: reject duplicate ids
    components_.push_back(std::move(comp));
}

bool SchematicDocument::removeComponent(const QString& id) {
    // TODO Stage 3: erase wires referencing this component
    for (auto it = components_.begin(); it != components_.end(); ++it) {
        if (it->id == id) {
            components_.erase(it);
            return true;
        }
    }
    return false;
}

void SchematicDocument::addWire(SchematicWire wire) {
    wires_.push_back(std::move(wire));
}

bool SchematicDocument::removeWire(const QString& id) {
    for (auto it = wires_.begin(); it != wires_.end(); ++it) {
        if (it->id == id) {
            wires_.erase(it);
            return true;
        }
    }
    return false;
}

const SchematicComponent* SchematicDocument::findComponent(const QString& id) const {
    for (const auto& c : components_) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

bool SchematicDocument::setComponentParameters(const QString& id,
                                              const QMap<QString, QVariant>& params) {
    for (auto& c : components_) {
        if (c.id == id) {
            c.parameters = params;
            return true;
        }
    }
    return false;
}

void SchematicDocument::clear() {
    components_.clear();
    wires_.clear();
}

} // namespace deepiri
