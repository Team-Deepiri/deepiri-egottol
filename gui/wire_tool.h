#pragma once

#include <QObject>
#include <QPointF>
#include <QList>

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
class QGraphicsScene;
QT_END_NAMESPACE

namespace deepiri {

class WireItem;
class SchematicScene;
class ComponentItem;

class WireTool : public QObject {
    Q_OBJECT

public:
    explicit WireTool(QObject* parent = nullptr);
    ~WireTool();

    void activate();
    void deactivate();
    bool is_active() const;

    void set_scene(SchematicScene* scene);
    SchematicScene* scene() const;

    void handle_click(QGraphicsSceneMouseEvent* event);
    void handle_move(QGraphicsSceneMouseEvent* event);
    void handle_release(QGraphicsSceneMouseEvent* event);

    void set_wire_color(const QColor& color);
    QColor wire_color() const;

signals:
    void wire_created(WireItem* wire);
    void wire_completed(WireItem* wire);

public slots:
    void cancel();

private:
    QPointF find_connection_point(const QPointF& pos) const;
    ComponentItem* find_component_at(const QPointF& pos) const;
    void update_preview_wire();

    class WireToolImpl;
    WireToolImpl* d;
};

}