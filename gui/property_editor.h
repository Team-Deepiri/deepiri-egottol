#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLineEdit;
class QLabel;
QT_END_NAMESPACE

namespace deepiri {

class ComponentItem;

// Shows the label and pin list of the currently selected schematic component,
// and lets the label be edited live.
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
    QLabel* pinsLabel_ = nullptr;
};

}
