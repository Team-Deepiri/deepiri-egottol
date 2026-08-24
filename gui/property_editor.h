#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLineEdit;
class QLabel;
QT_END_NAMESPACE

namespace deepiri {

class ComponentItem;

class PropertyEditor : public QWidget {
    Q_OBJECT

public:
    explicit PropertyEditor(QWidget* parent = nullptr);
    ~PropertyEditor() override;

public slots:
    void setComponent(ComponentItem* component);
    void clearComponent();

private:
    void rebuild();

    ComponentItem* component_ = nullptr;
    QFormLayout* form_ = nullptr;
    QLabel* typeLabel_ = nullptr;
    QLineEdit* labelEdit_ = nullptr;
    QLineEdit* valueEdit_ = nullptr;
    QLabel* pinsLabel_ = nullptr;
};

}
