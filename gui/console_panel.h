#pragma once

#include <QString>
#include <QWidget>

class QTextEdit;

/** Read-only sim log (Python: EgottolApp._console QTextEdit). */
namespace deepiri {

class ConsolePanel : public QWidget {
  Q_OBJECT

public:
  explicit ConsolePanel(QWidget *parent = nullptr);

public slots:
  void appendLine(const QString &line);
  void clear();

private:
  QTextEdit *text_ = nullptr;
};

} // namespace deepiri
