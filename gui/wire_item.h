#pragma once

#include <QGraphicsItem>
#include <QString>
#include <QList>
#include <QPointF>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace deepiri {

class WireItem : public QGraphicsItem {
public:
    explicit WireItem(QGraphicsItem* parent = nullptr);
    WireItem(const QPointF& start, const QPointF& end, QGraphicsItem* parent = nullptr);
    ~WireItem();

    void set_points(const QList<QPointF>& points);
    QList<QPointF> points() const;
    void add_point(const QPointF& point);

    QPointF start_point() const;
    QPointF end_point() const;
    void set_start_point(const QPointF& point);
    void set_end_point(const QPointF& point);

    void set_highlighted(bool highlighted);
    bool is_highlighted() const;

    void set_error(bool error);
    bool has_error() const;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    int type() const override;

signals:
    void points_changed(const QList<QPointF>& points);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    void update_path();

    class WireItemImpl;
    WireItemImpl* d;
};

}