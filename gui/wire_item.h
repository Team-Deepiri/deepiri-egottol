#pragma once

#include <QGraphicsObject>
#include <QList>
#include <QPointF>
#include <QString>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
class QPainterPath;
QT_END_NAMESPACE

namespace deepiri {

class WireItem : public QGraphicsObject {
  Q_OBJECT

public:
  explicit WireItem(QGraphicsItem *parent = nullptr);
  WireItem(const QPointF &start, const QPointF &end,
           QGraphicsItem *parent = nullptr);
  ~WireItem();

  void set_points(const QList<QPointF> &points);
  QList<QPointF> points() const;
  void add_point(const QPointF &point);

  QPointF start_point() const;
  QPointF end_point() const;
  void set_start_point(const QPointF &point);
  void set_end_point(const QPointF &point);

  void set_endpoints(const QString &fromComponentId, const QString &fromPort,
                     const QString &toComponentId, const QString &toPort);
  QString from_component_id() const;
  QString from_port() const;
  QString to_component_id() const;
  QString to_port() const;

  static QPainterPath orthogonal_path(const QPointF &start,
                                      const QPointF &end);

  void set_highlighted(bool highlighted);
  bool is_highlighted() const;

  void set_error(bool error);
  bool has_error() const;

  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget = nullptr) override;

  int type() const override;

signals:
  void points_changed(const QList<QPointF> &points);

protected:
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;

private:
  void update_path();

  class WireItemImpl;
  WireItemImpl *d;
};

} // namespace deepiri
