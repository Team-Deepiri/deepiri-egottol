#include "schematic_to_circuit.h"
#include "schematic_document.h"
#include "../core/circuit.h"
#include "../models/isrc.h"
#include "../models/resistor.h"
#include "../models/vsrc.h"

#include <QSet>

#include <functional>

namespace deepiri {

namespace {

QString portKey(const QString &componentId, const QString &port) {
    return componentId + ':' + port;
}

double parameter(const SchematicComponent &component, const QString &name,
                 double fallback) {
    const auto it = component.parameters.constFind(name);
    if (it == component.parameters.cend())
        return fallback;
    bool ok = false;
    const double value = it.value().toDouble(&ok);
    return ok ? value : fallback;
}

} // namespace

SchematicCircuitExport
buildCircuitFromSchematic(const SchematicDocument &document) {
    SchematicCircuitExport result;
    if (document.components().empty()) {
        result.error = QStringLiteral("The schematic is empty.");
        return result;
    }

    QMap<QString, QString> parent;
    for (const auto &component : document.components()) {
        for (const auto &port : component.ports) {
            const QString key = portKey(component.id, port.name);
            parent.insert(key, key);
        }
    }
    std::function<QString(const QString &)> root = [&](const QString &key) {
        const QString current = parent.value(key, key);
        if (current == key)
            return key;
        const QString representative = root(current);
        parent[key] = representative;
        return representative;
    };
    auto unite = [&](const QString &left, const QString &right) {
        if (!parent.contains(left) || !parent.contains(right))
            return;
        const QString leftRoot = root(left);
        const QString rightRoot = root(right);
        if (leftRoot != rightRoot)
            parent[rightRoot] = leftRoot;
    };
    for (const auto &wire : document.wires()) {
        unite(portKey(wire.fromComponentId, wire.fromPort),
              portKey(wire.toComponentId, wire.toPort));
    }

    QSet<QString> groundRoots;
    for (const auto &component : document.components()) {
        if (component.registryKey == QStringLiteral("GND") &&
            !component.ports.empty())
            groundRoots.insert(root(portKey(component.id,
                                            component.ports.front().name)));
    }
    if (groundRoots.isEmpty()) {
        result.error = QStringLiteral("DC analysis requires a GND component.");
        return result;
    }

    QMap<QString, size_t> rootNodes;
    size_t nextNode = 1;
    for (auto it = parent.cbegin(); it != parent.cend(); ++it) {
        const QString representative = root(it.key());
        size_t index = 0;
        if (!groundRoots.contains(representative)) {
            if (!rootNodes.contains(representative))
                rootNodes.insert(representative, nextNode++);
            index = rootNodes.value(representative);
        }
        result.portNodes.insert(it.key(), index);
        result.solverNodeMap[it.key().toStdString()] = index;
    }

    result.nodeLabels.fill(QString(), static_cast<qsizetype>(nextNode));
    result.nodeLabels[0] = QStringLiteral("GND");
    for (auto it = result.portNodes.cbegin(); it != result.portNodes.cend();
         ++it) {
        if (it.value() > 0 &&
            result.nodeLabels[static_cast<qsizetype>(it.value())].isEmpty())
            result.nodeLabels[static_cast<qsizetype>(it.value())] = it.key();
    }

    result.circuit = std::make_unique<Circuit>();
    for (size_t i = 0; i < static_cast<size_t>(result.nodeLabels.size()); ++i)
        result.circuit->addNode(result.nodeLabels[static_cast<qsizetype>(i)]
                                    .toStdString());

    for (const auto &component : document.components()) {
        auto node = [&](const QString &name) {
            return result.portNodes.value(portKey(component.id, name), 0);
        };
        std::shared_ptr<Device> device;
        if (component.registryKey == QStringLiteral("RES")) {
            const double resistance = parameter(component, "R", 1000.0);
            if (resistance <= 0.0) {
                result.error =
                    QStringLiteral("%1 has a non-positive resistance.")
                        .arg(component.id);
                result.circuit.reset();
                return result;
            }
            device = std::make_shared<Resistor>(component.id.toStdString(),
                                                resistance);
            device->setNodes(node("1"), node("2"));
        } else if (component.registryKey == QStringLiteral("IND")) {
            device =
                std::make_shared<Resistor>(component.id.toStdString(), 1e-9);
            device->setNodes(node("1"), node("2"));
        } else if (component.registryKey == QStringLiteral("VSRC")) {
            device = std::make_shared<Vsrc>(component.id.toStdString(),
                                            parameter(component, "V", 5.0));
            device->setNodes(node("+"), node("-"));
        } else if (component.registryKey == QStringLiteral("ISRC")) {
            device = std::make_shared<Isrc>(component.id.toStdString(),
                                            parameter(component, "I", 1e-3));
            device->setNodes(node("+"), node("-"));
        } else if (component.registryKey == QStringLiteral("GND") ||
                   component.registryKey == QStringLiteral("CAP")) {
            continue;
        } else {
            result.error =
                QStringLiteral("DC analysis does not yet support %1 (%2).")
                    .arg(component.id, component.registryKey);
            result.circuit.reset();
            return result;
        }
        result.circuit->addDevice(std::move(device));
    }
    return result;
}

} // namespace deepiri
