#include "component_item.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QDebug>

namespace deepiri {

class ComponentItem::ComponentItemImpl {
public:
    ComponentType type_;
    QString label_;
    QList<Pin> pins_;
    QColor selection_color_;
    QSizeF size_;
};

ComponentItem::ComponentItem(ComponentType type, const QString& label, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , d(new ComponentItemImpl)
{
    d->type_ = type;
    d->label_ = label;
    d->size_ = QSizeF(60, 40);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    update_pins();
}

ComponentItem::~ComponentItem() {
    delete d;
}

ComponentType ComponentItem::component_type() const {
    return d->type_;
}

QString ComponentItem::label() const {
    return d->label_;
}

void ComponentItem::set_label(const QString& label) {
    d->label_ = label;
    update();
}

QList<Pin> ComponentItem::pins() const {
    return d->pins_;
}

void ComponentItem::add_pin(const Pin& pin) {
    d->pins_.append(pin);
    update();
}

QPointF ComponentItem::pin_position(const QString& name) const {
    for (const Pin& pin : d->pins_) {
        if (pin.name == name) {
            return pin.position + pos();
        }
    }
    return QPointF();
}

QString ComponentItem::pin_at(const QPointF& pos) const {
    QPointF local_pos = mapFromScene(pos);
    for (const Pin& pin : d->pins_) {
        QPointF pin_scene_pos = mapToScene(pin.position);
        if (QLineF(local_pos, pin.position).length() < 10) {
            return pin.name;
        }
    }
    return QString();
}

void ComponentItem::set_selection_color(const QColor& color) {
    d->selection_color_ = color;
    update();
}

QColor ComponentItem::selection_color() const {
    return d->selection_color_;
}

QRectF ComponentItem::boundingRect() const {
    return QRectF(-d->size_.width() / 2, -d->size_.height() / 2,
                   d->size_.width(), d->size_.height());
}

void ComponentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(widget);
    
    QPen pen;
    pen.setWidth(2);
    
    if (option->state & QStyle::State_Selected) {
        pen.setColor(QColor(0, 120, 215));
        painter->setBrush(QColor(0, 120, 215, 30));
    } else {
        pen.setColor(Qt::white);
        painter->setBrush(QColor(45, 45, 48));
    }
    
    painter->setPen(pen);
    painter->drawRect(boundingRect());
    
    if (!d->label_.isEmpty()) {
        painter->setPen(Qt::white);
        painter->drawText(boundingRect(), Qt::AlignCenter, d->label_);
    }
    
    pen.setColor(Qt::yellow);
    painter->setPen(pen);
    for (const Pin& pin : d->pins_) {
        painter->drawEllipse(pin.position, 4, 4);
    }
}

int ComponentItem::type() const {
    return Type + 1;
}

QVariant ComponentItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionHasChanged) {
        update_pins();
    }
    return QGraphicsItem::itemChange(change, value);
}

void ComponentItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem::mousePressEvent(event);
}

void ComponentItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem::mouseReleaseEvent(event);
}

void ComponentItem::update_pins() {
    d->pins_.clear();
    
    switch (d->type_) {
        case ComponentType::AND_GATE:
        case ComponentType::OR_GATE:
        case ComponentType::NAND_GATE:
        case ComponentType::NOR_GATE:
        case ComponentType::XOR_GATE:
        case ComponentType::XNOR_GATE:
            add_pin({"A", QPointF(-30, -10), true, 1});
            add_pin({"B", QPointF(-30, 10), true, 1});
            add_pin({"Y", QPointF(30, 0), false, 1});
            break;
        case ComponentType::NOT_GATE:
            add_pin({"A", QPointF(-30, 0), true, 1});
            add_pin({"Y", QPointF(30, 0), false, 1});
            break;
        case ComponentType::D_FLIPFLOP:
        case ComponentType::JK_FLIPFLOP:
        case ComponentType::T_FLIPFLOP:
            add_pin({"D", QPointF(-30, -10), true, 1});
            add_pin({"CLK", QPointF(-30, 10), true, 1});
            add_pin({"Q", QPointF(30, -10), false, 1});
            add_pin({"QN", QPointF(30, 10), false, 1});
            break;
        case ComponentType::VCC:
            add_pin({"VCC", QPointF(0, 20), false, 1});
            break;
        case ComponentType::GROUND:
            add_pin({"GND", QPointF(0, 20), false, 1});
            break;
        default:
            break;
    }
}

}