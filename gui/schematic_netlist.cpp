#include "schematic_netlist.h"

#include "scene.h"
#include "component_item.h"
#include "wire_item.h"

#include <QLineF>
#include <QPointF>
#include <QString>
#include <cmath>
#include <map>
#include <sstream>
#include <vector>

namespace deepiri {

namespace {

constexpr qreal kPinSnapPx = 12.0;

struct PinRef {
    ComponentItem* component = nullptr;
    QString pinName;
    QPointF scenePos;
    int id = -1;
};

class UnionFind {
public:
    explicit UnionFind(int n) : parent_(n), rank_(n, 0) {
        for (int i = 0; i < n; ++i) parent_[i] = i;
    }

    int find(int x) {
        if (parent_[x] != x) parent_[x] = find(parent_[x]);
        return parent_[x];
    }

    void unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b) return;
        if (rank_[a] < rank_[b]) std::swap(a, b);
        parent_[b] = a;
        if (rank_[a] == rank_[b]) ++rank_[a];
    }

private:
    std::vector<int> parent_;
    std::vector<int> rank_;
};

int nearestPin(const std::vector<PinRef>& pins, const QPointF& p, qreal maxDist) {
    int best = -1;
    qreal bestDist = maxDist;
    for (const auto& pin : pins) {
        qreal d = QLineF(pin.scenePos, p).length();
        if (d <= bestDist) {
            bestDist = d;
            best = pin.id;
        }
    }
    return best;
}

bool isPassive(ComponentType t) {
    switch (t) {
        case ComponentType::RESISTOR:
        case ComponentType::VARISTOR:
        case ComponentType::THERMISTOR_NTC:
        case ComponentType::THERMISTOR_PTC:
        case ComponentType::TRIMMER:
        case ComponentType::CAPACITOR:
        case ComponentType::CAP_ELEC:
        case ComponentType::CAP_CER:
        case ComponentType::CAP_FILM:
        case ComponentType::CAP_TANT:
        case ComponentType::CAP_TRIM:
        case ComponentType::INDUCTOR:
        case ComponentType::IND_FERRITE:
        case ComponentType::IND_VAR:
            return true;
        default:
            return false;
    }
}

bool isCap(ComponentType t) {
    return t == ComponentType::CAPACITOR || t == ComponentType::CAP_ELEC ||
           t == ComponentType::CAP_CER || t == ComponentType::CAP_FILM ||
           t == ComponentType::CAP_TANT || t == ComponentType::CAP_TRIM;
}

bool isInd(ComponentType t) {
    return t == ComponentType::INDUCTOR || t == ComponentType::IND_FERRITE ||
           t == ComponentType::IND_VAR;
}

bool isDiodeLike(ComponentType t) {
    return t == ComponentType::LED || t == ComponentType::SCHOTTKY ||
           t == ComponentType::PHOTODIODE || t == ComponentType::DIODE_TUNNEL ||
           t == ComponentType::VARACTOR || t == ComponentType::LASER_DIODE;
}

char spicePrefix(ComponentType t) {
    if (isCap(t)) return 'C';
    if (isInd(t)) return 'L';
    if (t == ComponentType::SOURCE || t == ComponentType::VCC) return 'V';
    if (isDiodeLike(t)) return 'D';
    if (isPassive(t)) return 'R';
    return 'X';
}

std::string sanitizeName(const QString& label, char prefix, int fallbackIndex) {
    QString cleaned;
    for (QChar c : label) {
        if (c.isLetterOrNumber() || c == '_' || c == '-') {
            cleaned.append(c);
        }
    }
    if (cleaned.isEmpty()) {
        cleaned = QString("%1%2").arg(prefix).arg(fallbackIndex);
    } else if (!cleaned[0].isLetter() ||
               cleaned[0].toUpper() != QChar(prefix)) {
        // Ensure SPICE-style type prefix so the parser recognizes the device.
        cleaned = QChar(prefix) + cleaned;
    }
    return cleaned.toStdString();
}

}  // namespace

SchematicNetlistResult extractNetlistFromScene(const SchematicScene* scene) {
    SchematicNetlistResult result;
    if (!scene) {
        result.error = "No schematic scene";
        return result;
    }

    const QList<ComponentItem*> components = scene->components();
    const QList<WireItem*> wires = scene->wires();

    if (components.isEmpty()) {
        result.error = "Schematic is empty — insert components before running";
        return result;
    }

    std::vector<PinRef> pins;
    for (ComponentItem* comp : components) {
        for (const Pin& pin : comp->pins()) {
            PinRef ref;
            ref.component = comp;
            ref.pinName = pin.name;
            ref.scenePos = comp->mapToScene(pin.position);
            ref.id = static_cast<int>(pins.size());
            pins.push_back(ref);
        }
    }

    if (pins.empty()) {
        result.error = "No pins found on schematic components";
        return result;
    }

    UnionFind uf(static_cast<int>(pins.size()));

    // Connect pins that sit on top of each other.
    for (size_t i = 0; i < pins.size(); ++i) {
        for (size_t j = i + 1; j < pins.size(); ++j) {
            if (QLineF(pins[i].scenePos, pins[j].scenePos).length() <= kPinSnapPx) {
                uf.unite(static_cast<int>(i), static_cast<int>(j));
            }
        }
    }

    // Connect pins joined by wires.
    for (WireItem* wire : wires) {
        QList<QPointF> pts = wire->points();
        if (pts.size() < 2) continue;

        // Also union along intermediate points that land on pins (T-junctions).
        std::vector<int> touched;
        for (const QPointF& p : pts) {
            int id = nearestPin(pins, p, kPinSnapPx);
            if (id >= 0) touched.push_back(id);
        }
        // Always try endpoints even if slightly off.
        int startId = nearestPin(pins, pts.first(), kPinSnapPx * 1.5);
        int endId = nearestPin(pins, pts.last(), kPinSnapPx * 1.5);
        if (startId >= 0) touched.push_back(startId);
        if (endId >= 0) touched.push_back(endId);

        for (size_t i = 1; i < touched.size(); ++i) {
            uf.unite(touched[0], touched[i]);
        }
    }

    // Map union-find root → net name. Ground pins force net "0".
    std::map<int, std::string> rootToNet;
    int nextNet = 1;

    auto netNameForRoot = [&](int root, bool forceGnd) -> std::string {
        if (forceGnd) {
            rootToNet[root] = "0";
            return "0";
        }
        auto it = rootToNet.find(root);
        if (it != rootToNet.end()) return it->second;
        std::string name = "n" + std::to_string(nextNet++);
        rootToNet[root] = name;
        return name;
    };

    // First pass: mark ground roots.
    for (const auto& pin : pins) {
        if (pin.component->component_type() == ComponentType::GROUND) {
            netNameForRoot(uf.find(pin.id), true);
        }
    }

    auto pinNet = [&](ComponentItem* comp, const QString& pinName) -> std::string {
        for (const auto& pin : pins) {
            if (pin.component == comp && pin.pinName == pinName) {
                int root = uf.find(pin.id);
                auto it = rootToNet.find(root);
                if (it != rootToNet.end()) return it->second;
                return netNameForRoot(root, false);
            }
        }
        return "0";
    };

    std::ostringstream oss;
    oss << "* Extracted from schematic\n";
    int emitted = 0;
    int index = 0;

    for (ComponentItem* comp : components) {
        ++index;
        ComponentType type = comp->component_type();
        if (type == ComponentType::GROUND) {
            continue;  // net assignment only
        }

        QString value = comp->property("value", "1k");
        std::string name = sanitizeName(comp->label(), spicePrefix(type), index);
        QList<Pin> compPins = comp->pins();

        if (type == ComponentType::VCC) {
            // Single-pin rail → voltage source from rail net to ground.
            if (compPins.isEmpty()) continue;
            std::string n = pinNet(comp, compPins.first().name);
            oss << name << " " << n << " 0 " << value.toStdString() << "\n";
            ++emitted;
            continue;
        }

        if (type == ComponentType::SOURCE) {
            std::string plus = pinNet(comp, "+");
            std::string minus = pinNet(comp, "-");
            // If only one pin somehow, treat as rail.
            if (compPins.size() == 1) {
                plus = pinNet(comp, compPins.first().name);
                minus = "0";
            }
            oss << name << " " << plus << " " << minus << " " << value.toStdString() << "\n";
            ++emitted;
            continue;
        }

        if (compPins.size() < 2) {
            continue;
        }

        std::string a = pinNet(comp, compPins[0].name);
        std::string b = pinNet(comp, compPins[1].name);

        if (isCap(type)) {
            oss << name << " " << a << " " << b << " " << value.toStdString() << "\n";
        } else if (isInd(type)) {
            oss << name << " " << a << " " << b << " " << value.toStdString() << "\n";
        } else if (isDiodeLike(type)) {
            oss << name << " " << a << " " << b << "\n";
        } else if (isPassive(type) || spicePrefix(type) == 'R') {
            oss << name << " " << a << " " << b << " " << value.toStdString() << "\n";
        } else {
            // Unsupported digital/quantum blocks — skip silently for analog path.
            continue;
        }
        ++emitted;
    }

    if (emitted == 0) {
        result.error = "No simulatable analog components on the schematic";
        return result;
    }

    // Default analysis cards so a bare schematic can still run.
    oss << ".op\n";
    oss << ".tran 1u 1m\n";
    oss << ".ac dec 10 1 1Meg\n";
    oss << ".end\n";

    result.netlist = oss.str();
    result.ok = true;
    result.componentCount = emitted;
    result.netCount = nextNet - 1 + (rootToNet.count(0) ? 0 : 0);
    for (const auto& kv : rootToNet) {
        if (kv.second != "0") {
            // recount unique non-ground nets
        }
    }
    // Recompute net count cleanly.
    std::map<std::string, int> unique;
    for (const auto& kv : rootToNet) unique[kv.second] = 1;
    result.netCount = static_cast<int>(unique.size());
    return result;
}

}
