#include <tins/tins.h>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLayout>
#include "Gui.h"

using namespace Tins;

int main(int argc, char *argv[]) {
    auto* a = new QApplication(argc, argv);
    a->setStyle("Plastique");
    Gui w(nullptr);
    w.show();
    a->exec();
    return 0;
}