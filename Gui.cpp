#include "Gui.h"
#include "ui_Gui.h"
#include <QtWidgets/QMessageBox>
#include <QLayout>
#include <iostream>

using namespace Tins;
using namespace std;

Gui::Gui(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::Gui)
{
    ui->setupUi(this);

    ui->lineEdit_2->setValidator(new QIntValidator(0, 32, this));
    ui->lineEdit_4->setValidator(new QIntValidator(0, 32, this));
    ui->lineEdit_7->setValidator(new QIntValidator(0, 32, this));
    ui->lineEdit_9->setValidator(new QIntValidator(0, 100, this));

    vector<NetworkInterface> interfaces = NetworkInterface::all();
    for (auto &entry : interfaces){
        ui->comboBox->addItem(QString::fromStdString(entry.name()));
        ui->comboBox_2->addItem(QString::fromStdString(entry.name()));
    }

    ui->comboBox_3->addItem("none");

    ui->comboBox_2->setCurrentText("lo");

    ui->lineEdit->setText("1.1.1.1");
    ui->lineEdit_2->setText("24");
    ui->lineEdit_3->setText("2.2.2.2");
    ui->lineEdit_4->setText("24");

    ui->pushButton_6->setDisabled(true);
    ui->pushButton_4->setDisabled(true);

    ARPmodel = new QStringListModel(this);
    ui->listView->setModel(ARPmodel);

    ROUTEmodel = new QStringListModel(this);
    ui->listView_2->setModel(ROUTEmodel);

    RIPv2model = new QStringListModel(this);
    ui->listView_3->setModel(RIPv2model);

    routing_table = new Routing_table;

    arp_table = new ARP_table(this);
    arp_table->start();

    ripv2_database = new RIPv2_database(this);
    ripv2_database->routing_table = this->routing_table;

    one = new Interface(arp_table, routing_table, ripv2_database);
    two = new Interface(arp_table, routing_table, ripv2_database);

    one->setOtherInterface(two);
    two->setOtherInterface(one);

    ripv2_database->start();

    connect(arp_table, SIGNAL(printTable(QStringList)), this, SLOT(onARPprint(QStringList)));
    connect(routing_table, SIGNAL(printTable(QStringList)), this, SLOT(onROUTEprint(QStringList)));
    connect(ripv2_database, SIGNAL(print_database(QStringList)), this, SLOT(onRIPv2print(QStringList)));
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

        routing_table->deleteRecord("C", one->getInterface());

        ui->comboBox_3->clear();
        ui->comboBox_3->addItem("none");
        ui->comboBox_3->addItem(QString::fromStdString(interface));
        ui->comboBox_3->addItem(QString::fromStdString(two->getInterface()));

        IPv4Address netmask = IPv4Address::from_prefix_length(static_cast<uint32_t>(stoi(mask)));
        IPv4Address network = IPv4Address(ip) & netmask;

        auto* record = new Routing_table_record;
        record->network = network.to_string();
        record->netmask = static_cast<unsigned int>(stoi(mask));
        record->protocol = "C";
        record->interface = interface;
        record->administrativeDistance = 0;

        routing_table->addRecord(record);

        bool use_RIPv2 = ui->checkBox->isChecked();

        one->setIPv4(interface, ip, mask, use_RIPv2);

        ui->pushButton_6->setDisabled(false);
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

        routing_table->deleteRecord("C", two->getInterface());

        ui->comboBox_3->clear();
        ui->comboBox_3->addItem("none");
        ui->comboBox_3->addItem(QString::fromStdString(interface));
        ui->comboBox_3->addItem(QString::fromStdString(one->getInterface()));

        IPv4Address netmask = IPv4Address::from_prefix_length(static_cast<uint32_t>(stoi(mask)));
        IPv4Address network = IPv4Address(ip) & netmask;

        auto* record = new Routing_table_record;
        record->network = network.to_string();
        record->netmask = static_cast<unsigned int>(stoi(mask));
        record->protocol = "C";
        record->interface = interface;
        record->administrativeDistance = 0;

        routing_table->addRecord(record);

        bool use_RIPv2 = ui->checkBox_2->isChecked();
        two->setIPv4(interface, ip, mask, use_RIPv2);

        ui->pushButton_6->setDisabled(false);
    }
}

void Gui::on_pushButton_3_clicked() {
    if (one->getInterface() == "none" || two->getInterface() == "none"){
        QMessageBox messageBox;
        messageBox.critical(nullptr,"Error","Setup interfaces first");
        return;
    }

    arp_table->set_stop(false);

    arp_table->start();

    one->start();
    two->start();

    ui->pushButton_4->setDisabled(false);
    ui->pushButton->setDisabled(true);
    ui->pushButton_2->setDisabled(true);
    ui->pushButton_3->setDisabled(true);

    ui->comboBox_3->addItem(QString::fromStdString(one->getInterface()));
    ui->comboBox_3->addItem(QString::fromStdString(two->getInterface()));
}

void Gui::on_pushButton_4_clicked() {
    one->terminate();
    two->terminate();
    arp_table->set_stop(true);

    ui->pushButton->setDisabled(false);
    ui->pushButton_2->setDisabled(false);
    ui->pushButton_3->setDisabled(false);
    ui->pushButton_4->setDisabled(true);
}

void Gui::onARPprint(QStringList list) {
    this->ARPmodel->removeRows(0, this->ARPmodel->rowCount());
    this->ARPmodel->setStringList(list);
}

void Gui::on_pushButton_5_clicked() {
    if (one->isRunning())
        one->terminate();
    if (two->isRunning())
        two->terminate();

    arp_table->set_stop(true);

    exit(0);
}

void Gui::on_pushButton_6_clicked() {
    string targetIP = ui->lineEdit_5->text().toStdString();

    Routing_table_record* record = routing_table->findRecord(targetIP);
    if (record == nullptr)
        return;

    if (record->nextHop == "0.0.0.0")
        if (record->interface == one->getInterface()){
            one->sendPINGrequest(targetIP);
            return;
        } else {
            two->sendPINGrequest(targetIP);
            return;
        }

    if (record->interface == "none"){
        IPv4Address nextHop = record->nextHop;

        while (record->interface == "none"){
            record = routing_table->findRecord(record->nextHop);
            if (record == nullptr)
                return;
            if (record->interface != "none")
                break;
            nextHop = record->nextHop;
        }

        if (record->interface == one->getInterface()){
            one->sendPINGrequest(targetIP, &nextHop);
            return;
        } else {
            two->sendPINGrequest(targetIP, &nextHop);
            return;
        }
    }

    if (record->interface == one->getInterface()){
        one->sendPINGrequest(targetIP, &record->nextHop);
        return;
    } else {
        two->sendPINGrequest(targetIP, &record->nextHop);
        return;
    }
}

void Gui::on_spinBox_valueChanged(int i) {
    this->arp_table->set_timer((unsigned int)i);
}

void Gui::on_pushButton_7_clicked() {
    this->arp_table->clear();
}

void Gui::onROUTEprint(QStringList list) {
    this->ROUTEmodel->removeRows(0, this->ROUTEmodel->rowCount());
    this->ROUTEmodel->setStringList(list);
}

void Gui::on_pushButton_8_clicked() {
    auto* record = new Routing_table_record;
    unsigned int mask = (unsigned int) (std::stoi(ui->lineEdit_7->text().toStdString()));

    IPv4Address netmask = IPv4Address::from_prefix_length(mask);
    IPv4Address network = ui->lineEdit_6->text().toStdString();
    network = network & netmask;

    record->network = network;
    record->netmask = mask;
    record->administrativeDistance = 1;
    record->protocol = "S";
    if (!ui->lineEdit_8->text().isEmpty())
        record->nextHop = ui->lineEdit_8->text().toStdString();
    record->interface = ui->comboBox_3->currentText().toStdString();

    routing_table->addRecord(record);
}

void Gui::on_pushButton_9_clicked() {
    routing_table->clear();
}

void Gui::on_pushButton_10_clicked() {
    int index = stoi(ui->lineEdit_9->text().toStdString());
    routing_table->deleteRecord(index);
}

void Gui::onRIPv2print(QStringList list) {
    this->RIPv2model->removeRows(0, this->RIPv2model->rowCount());
    this->RIPv2model->setStringList(list);
}
