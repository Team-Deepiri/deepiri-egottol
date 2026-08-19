#pragma once

#include <QString>
#include <QPointF>
#include <QMap>
#include <QVariant>
#include <vector>

/**
 * schematic_document.h — Logical schematic model (Python: egottol/models/base.py Circuit).
 *
 * STAGE 1: Store what the user builds on the canvas. The scene holds QGraphicsItems;
 * this struct is the source of truth for save/load and (Stage 5) simulation export.
 *
 * YOU IMPLEMENT (Stage 3+):
 *   - addWire/removeWire with consistency checks
 *   - findComponent, wiresForComponent
 *   - toJson/fromJson for Stage 8 persistence
 */
namespace deepiri {

struct SchematicPort {
    QString name;
    QString direction; // "in", "out", "inout"
};

struct SchematicComponent {
    /** Unique instance id, e.g. "RES_a3f2b1" — matches ComponentItem label. */
    QString id;
    /** Registry key from SymbolLibrary, e.g. "RES", "VSRC", "AND". */
    QString registryKey;
    /** Human-readable name from library metadata. */
    QString displayName;
    /** Category string for palette coloring, e.g. "passive". */
    QString category;
    /** Symbol draw key — maps to SymbolRenderer / PortLayout (Python SYMBOL_KEY). */
    QString symbolKey;
    QPointF position;
    QMap<QString, QVariant> parameters;
    std::vector<SchematicPort> ports;
};

struct SchematicWire {
    QString id;
    QString fromComponentId;
    QString fromPort;
    QString toComponentId;
    QString toPort;
};

class SchematicDocument {
public:
    SchematicDocument();

    const std::vector<SchematicComponent>& components() const { return components_; }
    const std::vector<SchematicWire>& wires() const { return wires_; }

    /** Called by ComponentFactory after placing a ComponentItem on the scene. */
    void addComponent(SchematicComponent comp);

    /** Remove by id; Stage 3 should also remove attached wires. */
    bool removeComponent(const QString& id);

    /** Stage 3: append wire when user finishes port-to-port connection. */
    void addWire(SchematicWire wire);

    bool removeWire(const QString& id);

    const SchematicComponent* findComponent(const QString& id) const;
    std::vector<SchematicWire> wiresForComponent(const QString& id) const;
    bool setComponentPosition(const QString& id, const QPointF& position);

    /** Update params after PropertyEditor (Stage 4). */
    bool setComponentParameters(const QString& id, const QMap<QString, QVariant>& params);

    void clear();

private:
    std::vector<SchematicComponent> components_;
    std::vector<SchematicWire> wires_;
};

} // namespace deepiri
