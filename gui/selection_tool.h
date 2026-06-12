#pragma once

#include <QObject>
#include <QList>
#include <QPointF>
#include <QGraphicsItem>

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
class QGraphicsScene;
class QKeyEvent;
QT_END_NAMESPACE

namespace deepiri {

class SchematicScene;
class ComponentItem;
class WireItem;

class SelectionTool : public QObject {
    Q_OBJECT

public:
    explicit SelectionTool(QObject* parent = nullptr);
    ~SelectionTool();

    void activate();
    void deactivate();
    bool is_active() const;

    void set_scene(SchematicScene* scene);
    SchematicScene* scene() const;

    void handle_click(QGraphicsSceneMouseEvent* event);
    void handle_move(QGraphicsSceneMouseEvent* event);
    void handle_release(QGraphicsSceneMouseEvent* event);
    void handle_key_press(QKeyEvent* event);

    void select_all();
    void clear_selection();
    void delete_selected();
    void copy_selected();
    void paste_selected();

    QList<QGraphicsItem*> selected_items() const;

signals:
    void activated();
    void deactivated();
    void selection_changed(const QList<QGraphicsItem*>& items);

public slots:
    void cancel();

private:
    ComponentItem* find_component_underMouse(const QPointF& pos) const;
    WireItem* find_wire_underMouse(const QPointF& pos) const;
    void update_selection_rect();

    class SelectionToolImpl;
    SelectionToolImpl* d;
};

}