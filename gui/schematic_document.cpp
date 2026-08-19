#include "schematic_document.h"

#include <algorithm>

namespace deepiri {

SchematicDocument::SchematicDocument() = default;

void SchematicDocument::addComponent(SchematicComponent comp) {
    if (comp.id.isEmpty() || findComponent(comp.id))
        return;
    components_.push_back(std::move(comp));
}

bool SchematicDocument::removeComponent(const QString& id) {
    wires_.erase(
        std::remove_if(wires_.begin(), wires_.end(),
                       [&id](const SchematicWire &wire) {
                           return wire.fromComponentId == id ||
                                  wire.toComponentId == id;
                       }),
        wires_.end());
    for (auto it = components_.begin(); it != components_.end(); ++it) {
        if (it->id == id) {
            components_.erase(it);
            return true;
        }
    }
    return false;
}

void SchematicDocument::addWire(SchematicWire wire) {
    if (wire.id.isEmpty() || wire.fromComponentId == wire.toComponentId &&
                                 wire.fromPort == wire.toPort)
        return;
    if (std::any_of(wires_.begin(), wires_.end(),
                    [&wire](const SchematicWire &existing) {
                        return existing.id == wire.id;
                    }))
        return;
    const auto *from = findComponent(wire.fromComponentId);
    const auto *to = findComponent(wire.toComponentId);
    if (!from || !to)
        return;
    auto hasPort = [](const SchematicComponent *component,
                      const QString &name) {
        return std::any_of(component->ports.begin(), component->ports.end(),
                           [&name](const SchematicPort &port) {
                               return port.name == name;
                           });
    };
    if (!hasPort(from, wire.fromPort) || !hasPort(to, wire.toPort))
        return;
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

std::vector<SchematicWire>
SchematicDocument::wiresForComponent(const QString &id) const {
    std::vector<SchematicWire> result;
    for (const auto &wire : wires_) {
        if (wire.fromComponentId == id || wire.toComponentId == id)
            result.push_back(wire);
    }
    return result;
}

bool SchematicDocument::setComponentPosition(const QString &id,
                                             const QPointF &position) {
    for (auto &component : components_) {
        if (component.id == id) {
            component.position = position;
            return true;
        }
    }
    return false;
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
