#pragma once

#include <QGraphicsView>
#include <QPointF>

QT_BEGIN_NAMESPACE
class QWheelEvent;
class QKeyEvent;
class QMouseEvent;
class QGraphicsScene;
QT_END_NAMESPACE

namespace deepiri {

class SchematicScene;

class SchematicView : public QGraphicsView {
  Q_OBJECT

public:
  explicit SchematicView(QWidget *parent = nullptr);
  explicit SchematicView(SchematicScene *scene, QWidget *parent = nullptr);
  ~SchematicView();

  void set_schematic_scene(SchematicScene *scene);
  SchematicScene *schematic_scene() const;

  void zoom_in();
  void zoom_out();
  void zoom_fit();
  void zoom_reset();

  qreal zoom_factor() const;
  void set_zoom_factor(qreal factor);

  void pan_to(const QPointF &center);
  QPointF center() const;

  void set_grid_visible(bool visible);
  bool is_grid_visible() const;

signals:
  void zoom_changed(qreal factor);
  void pan_changed(const QPointF &center);

public slots:
  void handle_zoom_in();
  void handle_zoom_out();

protected:
  void wheelEvent(QWheelEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;

private:
  void initialize();
  void update_zoom();

  class SchematicViewImpl;
  SchematicViewImpl *d;
};

} // namespace deepiri