#include "Gui.h"
#include "ui_Gui.h"
#include <QtWidgets/QMessageBox>
#include <QLayout>
#include <iostream>

using namespace Tins;
using namespace std;

Gui::Gui(QWidget *parent) :
        QMainWindow(nullptr),
        ui(new Ui::Gui)
{
    ui->setupUi(this);
    this->setWindowTitle("Router");

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
    ui->checkBox->setDisabled(true);
    ui->checkBox_2->setDisabled(true);

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

    ripv2_database->one = one;
    ripv2_database->two = two;

    ripv2_database->start();

    connect(arp_table, SIGNAL(printTable(QStringList)), this, SLOT(onARPprint(QStringList)));
    connect(routing_table, SIGNAL(printTable(QStringList)), this, SLOT(onROUTEprint(QStringList)));
    connect(ripv2_database, SIGNAL(print_database(QStringList, unsigned int)), this, SLOT(onRIPv2print(QStringList, unsigned int)));
    connect(ui->checkBox, SIGNAL(toggled(bool)), one, SLOT(onUseRipv2(bool)));
    connect(ui->checkBox_2, SIGNAL(toggled(bool)), two, SLOT(onUseRipv2(bool)));
    connect(ui->checkBox, SIGNAL(toggled(bool)), ripv2_database, SLOT(set_ripv2_one(bool)));
    connect(ui->checkBox_2, SIGNAL(toggled(bool)), ripv2_database, SLOT(set_ripv2_two(bool)));
    connect(ui->spinBox_2, SIGNAL(valueChanged(int)), ripv2_database, SLOT(set_timerUpdate(int)));
    connect(ui->spinBox_4, SIGNAL(valueChanged(int)), ripv2_database, SLOT(set_timerInvalid(int)));
    connect(ui->spinBox_5, SIGNAL(valueChanged(int)), ripv2_database, SLOT(set_timerHolddown(int)));
    connect(ui->spinBox_6, SIGNAL(valueChanged(int)), ripv2_database, SLOT(set_timerFlush(int)));
    connect(routing_table, SIGNAL(findReplacement(IPv4Address, unsigned int)), ripv2_database, SLOT(findReplacement(IPv4Address, unsigned int)));
    connect(one, SIGNAL(statistics_changed()), this, SLOT(onStatisticsChanged()));
    connect(two, SIGNAL(statistics_changed()), this, SLOT(onStatisticsChanged()));
    connect(one, SIGNAL(successful_PING()), this, SLOT(onSuccessfulPING()));
    connect(two, SIGNAL(successful_PING()), this, SLOT(onSuccessfulPING()));
    connect(one, SIGNAL(unreachable()), this, SLOT(onUnreachable()));
    connect(two, SIGNAL(unreachable()), this, SLOT(onUnreachable()));

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
        record->metric = 0;

        routing_table->addRecord(record);

//        ripv2_database->connected_one = network;
//        ripv2_database->interface_one = interface;
//        ripv2_database->address_one = ip;

        one->setIPv4(interface, ip, (unsigned int)stoi(mask));

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
        record->metric = 0;

        routing_table->addRecord(record);

        two->setIPv4(interface, ip, (unsigned int)stoi(mask));

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
    ui->checkBox->setDisabled(false);
    ui->checkBox_2->setDisabled(false);
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
    if (record == nullptr){
        QMessageBox messageBox;
        messageBox.critical(nullptr, "Unreachable", "Network is unreachable");
        return;
    }


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
    record->metric = 0;
    record->protocol = "S";
    if (!ui->lineEdit_8->text().isEmpty())
        record->nextHop = ui->lineEdit_8->text().toStdString();
    record->interface = ui->comboBox_3->currentText().toStdString();

    routing_table->addRecord(record);
}

void Gui::on_pushButton_9_clicked() {
    routing_table->clear();

    string interface_one = ui->comboBox->currentText().toStdString();
    string ip_one = ui->lineEdit->text().toStdString();
    string mask_one = ui->lineEdit_2->text().toStdString();

    IPv4Address netmask_one = IPv4Address::from_prefix_length(static_cast<uint32_t>(stoi(mask_one)));
    IPv4Address network_one = IPv4Address(ip_one) & netmask_one;

    string interface_two = ui->comboBox_2->currentText().toStdString();
    string ip_two = ui->lineEdit_3->text().toStdString();
    string mask_two = ui->lineEdit_4->text().toStdString();

    IPv4Address netmask_two = IPv4Address::from_prefix_length(static_cast<uint32_t>(stoi(mask_two)));
    IPv4Address network_two = IPv4Address(ip_two) & netmask_two;

    auto *record_one = new Routing_table_record;
    record_one->network = network_one;
    record_one->netmask = static_cast<unsigned int>(stoi(mask_one));
    record_one->interface = interface_one;
    record_one->protocol = "C";
    record_one->administrativeDistance = 0;
    record_one->metric = 0;

    routing_table->addRecord(record_one);

    auto *record_two = new Routing_table_record;
    record_two->network = network_two;
    record_two->netmask = static_cast<unsigned int>(stoi(mask_two));
    record_two->interface = interface_two;
    record_two->protocol = "C";
    record_two->administrativeDistance = 0;
    record_two->metric = 0;

    routing_table->addRecord(record_two);
}

void Gui::on_pushButton_10_clicked() {
    int index = stoi(ui->lineEdit_9->text().toStdString());
    routing_table->deleteRecord(index);
}

void Gui::onRIPv2print(QStringList list, unsigned int update_in) {
    ui->lcdNumber_9->display((int)update_in);
    RIPv2model->removeRows(0, this->RIPv2model->rowCount());
    RIPv2model->setStringList(list);
}

void Gui::onStatisticsChanged() {
    ui->lcdNumber->display((int) one->statistics_in.EthernetII);
    ui->lcdNumber_2->display((int) one->statistics_in.ARP_REQ);
    ui->lcdNumber_10->display((int) one->statistics_in.ARP_REP);
    ui->lcdNumber_3->display((int) one->statistics_in.IP);
    ui->lcdNumber_4->display((int) one->statistics_in.ICMP_REQ);
    ui->lcdNumber_5->display((int) one->statistics_in.ICMP_REP);
    ui->lcdNumber_6->display((int) one->statistics_in.UDP);
    ui->lcdNumber_7->display((int) one->statistics_in.RIPv2_REQ);
    ui->lcdNumber_8->display((int) one->statistics_in.RIPv2_REP);

    ui->lcdNumber_17->display((int) one->statistics_out.EthernetII);
    ui->lcdNumber_18->display((int) one->statistics_out.ARP_REQ);
    ui->lcdNumber_11->display((int) one->statistics_out.ARP_REP);
    ui->lcdNumber_19->display((int) one->statistics_out.IP);
    ui->lcdNumber_20->display((int) one->statistics_out.ICMP_REQ);
    ui->lcdNumber_21->display((int) one->statistics_out.ICMP_REP);
    ui->lcdNumber_22->display((int) one->statistics_out.UDP);
    ui->lcdNumber_23->display((int) one->statistics_out.RIPv2_REQ);
    ui->lcdNumber_24->display((int) one->statistics_out.RIPv2_REP);

    ui->lcdNumber_25->display((int) two->statistics_in.EthernetII);
    ui->lcdNumber_26->display((int) two->statistics_in.ARP_REQ);
    ui->lcdNumber_12->display((int) two->statistics_in.ARP_REP);
    ui->lcdNumber_27->display((int) two->statistics_in.IP);
    ui->lcdNumber_28->display((int) two->statistics_in.ICMP_REQ);
    ui->lcdNumber_29->display((int) two->statistics_in.ICMP_REP);
    ui->lcdNumber_30->display((int) two->statistics_in.UDP);
    ui->lcdNumber_31->display((int) two->statistics_in.RIPv2_REQ);
    ui->lcdNumber_32->display((int) two->statistics_in.RIPv2_REP);

    ui->lcdNumber_33->display((int) two->statistics_out.EthernetII);
    ui->lcdNumber_34->display((int) two->statistics_out.ARP_REQ);
    ui->lcdNumber_13->display((int) two->statistics_out.ARP_REP);
    ui->lcdNumber_35->display((int) two->statistics_out.IP);
    ui->lcdNumber_36->display((int) two->statistics_out.ICMP_REQ);
    ui->lcdNumber_37->display((int) two->statistics_out.ICMP_REP);
    ui->lcdNumber_38->display((int) two->statistics_out.UDP);
    ui->lcdNumber_39->display((int) two->statistics_out.RIPv2_REQ);
    ui->lcdNumber_40->display((int) two->statistics_out.RIPv2_REP);
}

void Gui::onSuccessfulPING() {
    QMessageBox messageBox;
    messageBox.information(nullptr, "PING", "Ping successful");
}

void Gui::onUnreachable() {
    QMessageBox messageBox;
    messageBox.critical(nullptr, "Unreachable", "Destination is unreachable");
}
