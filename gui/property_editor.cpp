#include "property_editor.h"
#include "component_item.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QStringList>

namespace deepiri {

PropertyEditor::PropertyEditor(QWidget* parent) : QWidget(parent) {
    form_ = new QFormLayout(this);

    typeLabel_ = new QLabel("—", this);
    labelEdit_ = new QLineEdit(this);
    pinsLabel_ = new QLabel("—", this);
    pinsLabel_->setWordWrap(true);

    form_->addRow("Type", typeLabel_);
    form_->addRow("Label", labelEdit_);
    form_->addRow("Pins", pinsLabel_);

    connect(labelEdit_, &QLineEdit::editingFinished, this, [this]() {
        if (component_) {
            component_->set_label(labelEdit_->text());
        }
    });

    clearComponent();
}

PropertyEditor::~PropertyEditor() = default;

void PropertyEditor::setComponent(ComponentItem* component) {
    component_ = component;
    rebuild();
}

void PropertyEditor::clearComponent() {
    component_ = nullptr;
    rebuild();
}

void PropertyEditor::rebuild() {
    bool hasComponent = component_ != nullptr;
    labelEdit_->setEnabled(hasComponent);

    if (!hasComponent) {
        typeLabel_->setText("No component selected");
        labelEdit_->setText("");
        pinsLabel_->setText("—");
        return;
    }

    typeLabel_->setText(QString("Component type #%1").arg(static_cast<int>(component_->component_type())));
    labelEdit_->setText(component_->label());

    QStringList pinNames;
    for (const auto& pin : component_->pins()) {
        pinNames << pin.name;
    }
    pinsLabel_->setText(pinNames.isEmpty() ? "(none)" : pinNames.join(", "));
}

}
