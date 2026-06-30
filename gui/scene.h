#pragma once

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QList>
#include <QString>
#include <QPointF>

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
class QMenu;
class QAction;
QT_END_NAMESPACE

namespace deepiri {

class SchematicDocument;
class ComponentItem;
class WireItem;
class SelectionTool;
class WireTool;

class SchematicScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit SchematicScene(QObject* parent = nullptr);
    ~SchematicScene();

    void add_component(ComponentItem* component);
    void add_wire(WireItem* wire);
    void remove_component(ComponentItem* component);
    void remove_wire(WireItem* wire);

    QList<ComponentItem*> components() const;
    QList<WireItem*> wires() const;
    ComponentItem* component_at(const QPointF& pos) const;
    QList<WireItem*> wires_at(const QPointF& pos) const;

    void set_grid_size(double size);
    double grid_size() const;
    void set_snap_to_grid(bool snap);
    bool snap_to_grid() const;

    void set_selection_tool(SelectionTool* tool);
    void set_wire_tool(WireTool* tool);

    QPointF snap_position(const QPointF& pos) const;
    QPointF grid_position(const QPointF& pos) const;

    /** Attach logical model (Python SchematicScene._circuit). */
    void set_document(SchematicDocument* document);
    SchematicDocument* document() const;

    /** Palette/toolbar sets key; next canvas click places component (Python set_place_mode). */
    void set_place_mode(const QString& registryKey);
    void clear_place_mode();
    QString place_mode() const;

    /** Reset scene + caller should clear document (Python clear_canvas). */
    void clear_canvas();

    /** Esc: exit place mode and cancel in-progress wire (Python view keyPressEvent). */
    void handle_escape();

signals:
    void placeModeChanged(const QString& modeLabel);
    void component_added(ComponentItem* component);
    void component_removed(ComponentItem* component);
    void wire_added(WireItem* wire);
    void wire_removed(WireItem* wire);
    void selection_changed(const QList<QGraphicsItem*>& items);

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void draw_grid(QPainter* painter);

    class SchematicSceneImpl;
    SchematicSceneImpl* d;
};

}