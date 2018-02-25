#include <tins/tins.h>
#include <QtWidgets/QApplication>
#include "Gui.h"

using namespace Tins;

int main(int argc, char *argv[]) {
//    qputenv("XDG_RUNTIME_DIR", "/tmp/runtime-root");

    auto* a = new QApplication(argc, argv);
    Gui w;
    w.show();
    a->exec();
    return 0;
}