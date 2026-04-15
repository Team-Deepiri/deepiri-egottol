#pragma once

#include <QGraphicsItem>
#include <QString>
#include <QList>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QColor>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace deepiri {

enum class ComponentType {
    AND_GATE,
    OR_GATE,
    NOT_GATE,
    NAND_GATE,
    NOR_GATE,
    XOR_GATE,
    XNOR_GATE,
    D_FLIPFLOP,
    JK_FLIPFLOP,
    T_FLIPFLOP,
    SOURCE,
    GROUND,
    LED,
    RESISTOR,
    CAPACITOR,
    INDUCTOR,
    VCC,
    BUFFER,
    CUSTOM
};

struct Pin {
    QString name;
    QPointF position;
    bool is_input;
    int bit_width;
};

class ComponentItem : public QGraphicsItem {
public:
    explicit ComponentItem(ComponentType type, const QString& label = QString(), QGraphicsItem* parent = nullptr);
    ~ComponentItem();

    ComponentType component_type() const;
    QString label() const;
    void set_label(const QString& label);

    QList<Pin> pins() const;
    void add_pin(const Pin& pin);
    QPointF pin_position(const QString& name) const;
    QString pin_at(const QPointF& pos) const;

    void set_selection_color(const QColor& color);
    QColor selection_color() const;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    int type() const override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void update_pins();

    class ComponentItemImpl;
    ComponentItemImpl* d;
};

}