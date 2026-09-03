#pragma once

#include <QDialog>
#include <QMap>
#include <QString>
#include <QVariant>

class QLineEdit;

namespace deepiri {

class PropertyEditor : public QDialog {
public:
  explicit PropertyEditor(const QString &componentId,
                          const QMap<QString, QVariant> &parameters,
                          QWidget *parent = nullptr);
  QMap<QString, QVariant> parameters() const;

public slots:
  void accept() override;

private:
  QMap<QString, QLineEdit *> editors_;
};

} // namespace deepiri