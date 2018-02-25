#ifndef GUI_H
#define GUI_H

#include <QMainWindow>
#include <QtCore/QStringListModel>
#include "ARP_table.h"
#include "Interface.h"

namespace Ui {
    class Gui;
}

class Gui : public QMainWindow
{
Q_OBJECT

public:
    explicit Gui(QWidget *parent = 0);
    ~Gui();

private:
    Ui::Gui *ui;

    ARP_table* arp_table;
    Interface* one;
    Interface* two;

    QStringListModel* model;

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_6_clicked();

public slots:
    void onARPprint(QStringList list);

};

#endif // GUI_H
