#include "property_editor.h"

#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

namespace deepiri {

PropertyEditor::PropertyEditor(const QString &componentId,
                               const QMap<QString, QVariant> &parameters,
                               QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(tr("Properties — %1").arg(componentId));
  setModal(true);

  auto *layout = new QVBoxLayout(this);
  auto *form = new QFormLayout();
  if (parameters.isEmpty())
    form->addRow(new QLabel(tr("This component has no editable parameters."),
                            this));
  for (auto it = parameters.cbegin(); it != parameters.cend(); ++it) {
    auto *editor = new QLineEdit(it.value().toString(), this);
    if (it.value().canConvert<double>()) {
      auto *validator = new QDoubleValidator(editor);
      validator->setNotation(QDoubleValidator::ScientificNotation);
      editor->setValidator(validator);
    }
    editors_.insert(it.key(), editor);
    form->addRow(it.key(), editor);
  }
  layout->addLayout(form);

  auto *buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                           Qt::Horizontal, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);
}

QMap<QString, QVariant> PropertyEditor::parameters() const {
  QMap<QString, QVariant> values;
  for (auto it = editors_.cbegin(); it != editors_.cend(); ++it) {
    bool ok = false;
    const double number = it.value()->text().toDouble(&ok);
    values.insert(it.key(), ok ? QVariant(number)
                               : QVariant(it.value()->text()));
  }
  return values;
}

void PropertyEditor::accept() {
  for (auto it = editors_.cbegin(); it != editors_.cend(); ++it) {
    if (!it.value()->hasAcceptableInput()) {
      QMessageBox::warning(this, tr("Invalid value"),
                           tr("%1 must be a valid number.").arg(it.key()));
      it.value()->setFocus();
      it.value()->selectAll();
      return;
    }
  }
  QDialog::accept();
}

} // namespace deepiri