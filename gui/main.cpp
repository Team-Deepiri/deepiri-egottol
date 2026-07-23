#include <QApplication>
#include "main_window.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Deepiri Egottol");
    QApplication::setOrganizationName("Deepiri");

    deepiri::MainWindow window;
    window.show();

    return app.exec();
}
