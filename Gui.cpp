#include "Gui.h"
#include "ui_Gui.h"
#include <QtWidgets/QMessageBox>
#include <QLayout>
#include <QtGui/QStandardItemModel>
#include <iostream>

using namespace Tins;
using namespace std;

Gui::Gui(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::Gui)
{
    ui->setupUi(this);

    vector<NetworkInterface> interfaces = NetworkInterface::all();
    for (auto &entry : interfaces){
        ui->comboBox->addItem(QString::fromStdString(entry.name()));
        ui->comboBox_2->addItem(QString::fromStdString(entry.name()));
    }

    ui->comboBox_2->setCurrentText("lo");

    ui->lineEdit_3->setText("1.1.1.1");
    ui->lineEdit_4->setText("255.255.255.0");

    this->model = new QStringListModel(this);
    ui->listView->setModel(model);


    this->arp_table = new ARP_table(this);
    this->arp_table->start();

    this->one = new Interface(this->arp_table);
    this->two = new Interface(this->arp_table);


    connect(arp_table, SIGNAL(printTable(QStringList)), this, SLOT(onARPprint(QStringList)));
}

Gui::~Gui()
{
    delete ui;
}

void Gui::on_pushButton_clicked() {
    if (ui->lineEdit->text().isEmpty() || ui->lineEdit_2->text().isEmpty()){
        QMessageBox messageBox;
        messageBox.critical(nullptr,"Error","Bad format\t");
    } else {
        string interface = ui->comboBox->currentText().toStdString();
        string ip = ui->lineEdit->text().toStdString();
        string mask = ui->lineEdit_2->text().toStdString();

        this->one->setIPv4(interface, ip, mask);
    }
}

void Gui::on_pushButton_2_clicked() {
    if (ui->lineEdit_3->text().isEmpty() || ui->lineEdit_4->text().isEmpty()){
        QMessageBox messageBox;
        messageBox.critical(nullptr,"Error","Bad format\t");
    } else {
        string interface = ui->comboBox_2->currentText().toStdString();
        string ip = ui->lineEdit_3->text().toStdString();
        string mask = ui->lineEdit_4->text().toStdString();

        this->two->setIPv4(interface, ip, mask);
    }
}

void Gui::on_pushButton_3_clicked() {
    if (this->one->getInterface() == "none" || this->two->getInterface() == "none"){
        QMessageBox messageBox;
        messageBox.critical(nullptr,"Error","Setup interfaces first");
        return;
    }
    this->one->setStop(false);
    this->two->setStop(false);

    this->one->start();
    this->two->start();

    this->arp_table->set_stop(false);
    this->arp_table->start();
}

void Gui::on_pushButton_4_clicked() {
    this->one->setStop(true);
    this->two->setStop(true);
    this->arp_table->set_stop(true);
}

void Gui::onARPprint(QStringList list) {
    this->model->removeRows(0, this->model->rowCount());
    this->model->setStringList(list);
}

void Gui::on_pushButton_5_clicked() {
    this->one->setStop(true);
    this->two->setStop(true);
    this->arp_table->set_stop(true);
    exit(0);
}

void Gui::on_pushButton_6_clicked() {
    this->one->sendARPrequest(IPv4Address(ui->lineEdit_5->text().toStdString()));
}
