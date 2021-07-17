#include <QApplication>
#include <array>
#include <glm/glm.hpp>
#include <iostream>

#include "main_window.h"
#include "raw_reader.h"
#include "volume_rendering.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("upupming");
    app.setApplicationName("Volume Rendering (threshold)");
    app.setApplicationVersion(QT_VERSION_STR);

#if _DEBUG
    std::cout << "Debug mode" << std::endl;
#else
    std::cout << "Release mode" << std::endl;
#endif

#ifndef QT_NO_OPENGL
    MainWindow mainWin;
    mainWin.show();

#else
    QLabel note("OpenGL Support required");
    note.show();
#endif
    return app.exec();
}
