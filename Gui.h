#ifndef GUI_H
#define GUI_H

#include <QMainWindow>
#include <QtCore/QStringListModel>
#include "ARP_table.h"
#include "Interface.h"
#include "Routing_table.h"

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
    Routing_table* routing_table;
    RIPv2_database* ripv2_database;

    Interface* one;
    Interface* two;

    QStringListModel* ARPmodel;
    QStringListModel* ROUTEmodel;
    QStringListModel* RIPv2model;

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_6_clicked();
    void on_pushButton_7_clicked();
    void on_pushButton_8_clicked();
    void on_pushButton_9_clicked();
    void on_pushButton_10_clicked();
    void on_spinBox_valueChanged(int i);

public slots:
    void onARPprint(QStringList list);
    void onROUTEprint(QStringList list);
    void onRIPv2print(QStringList list);

};

#endif // GUI_H
