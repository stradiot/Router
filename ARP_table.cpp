#include <iostream>
#include <QtCore/QTextStream>
#include "ARP_record.h"
#include "ARP_table.h"

void ARP_table::run() {
    while (!stop){
        print();
        std::unique_lock<std::mutex> guard(mutex);

        unsigned long i = 0;
        while (i < records.size()) {
            auto* act = records.at(i);
            if (act->timer == 0) {
                records.erase(records.begin() + i);
            } else {
                act->timer--;
                i++;
            }
        }
        guard.unlock();
        sleep(1);
    }
}

void ARP_table::add_record(ARP_record *record) {
    for (auto act : records) {
        if (act->ip == record->ip) {
            if (act->mac != record->mac)
                act->mac = record->mac;
            return;
        }
    }

    record->timer = timer;
    records.push_back(record);
}

void ARP_table::print() {
    QStringList list;
    QString header = "    IP\t          MAC\t\tTIMER";
    list << header << "";

    for (auto act : records) {
        QString ip = QString::fromStdString(act->ip.to_string());
        QString mac = QString::fromStdString(act->mac.to_string());
        QString timer = QString::number(act->timer);

        QString record = ip + "\t" + mac + "\t   " + timer;

        list << record;
    }

    emit printTable(list);
}

void ARP_table::set_timer(unsigned int value) {
    this->timer = value;
}

void ARP_table::set_stop(bool value) {
    this->stop = value;
}

void ARP_table::clear() {
    std::unique_lock<std::mutex> guard(mutex);
    records.clear();
    guard.unlock();
}

HWAddress<6>* ARP_table::findRecord(IPv4Address targetIP) {
    auto* targetMAC = new HWAddress<6>;

    for (auto act : records) {
        if (act->ip == targetIP){
            *targetMAC = act->mac;
            return targetMAC;
        }
    }

    return nullptr;
}

ARP_table::ARP_table(QObject *parent) : QThread(parent) {

}
