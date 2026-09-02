#include "schematic_mermaid.h"

#include "scene.h"
#include "component_item.h"
#include "wire_item.h"

#include <QString>
#include <sstream>

namespace deepiri {

namespace {

const char* typeName(ComponentType t) {
    switch (t) {
        case ComponentType::RESISTOR: return "R";
        case ComponentType::CAPACITOR: return "C";
        case ComponentType::INDUCTOR: return "L";
        case ComponentType::SOURCE: return "V";
        case ComponentType::GROUND: return "GND";
        case ComponentType::LED: return "D";
        case ComponentType::VCC: return "VCC";
        default: return "X";
    }
}

std::string safeId(const QString& label, int index) {
    QString s;
    for (QChar c : label) {
        if (c.isLetterOrNumber() || c == '_') s.append(c);
    }
    if (s.isEmpty()) s = QString("n%1").arg(index);
    return s.toStdString();
}

}  // namespace

std::string schematicToMermaid(const SchematicScene* scene) {
    std::ostringstream oss;
    oss << "flowchart LR\n";
    if (!scene) {
        oss << "  empty[Empty schematic]\n";
        return oss.str();
    }

    const auto comps = scene->components();
    int i = 0;
    for (ComponentItem* c : comps) {
        ++i;
        std::string id = safeId(c->label(), i);
        std::string label = c->label().toStdString();
        if (label.empty()) label = typeName(c->component_type());
        QString value = c->property("value");
        if (!value.isEmpty()) {
            label += "\\n" + value.toStdString();
        }
        oss << "  " << id << "[\"" << typeName(c->component_type()) << ": " << label << "\"]\n";
    }

    // Wires as anonymous edges between nearest components by endpoint proximity.
    const auto wires = scene->wires();
    for (WireItem* w : wires) {
        auto pts = w->points();
        if (pts.size() < 2) continue;
        ComponentItem* a = scene->component_at(pts.first());
        ComponentItem* b = scene->component_at(pts.last());
        if (!a || !b || a == b) continue;
        oss << "  " << safeId(a->label(), 0) << " --- " << safeId(b->label(), 0) << "\n";
    }

    if (comps.isEmpty()) {
        oss << "  empty[Empty schematic]\n";
    }
    return oss.str();
}

}
