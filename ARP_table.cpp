#include <iostream>
#include <QtCore/QTextStream>
#include "ARP_record.h"
#include "ARP_table.h"

void ARP_table::run() {
    while (!this->stop){
        this->print();
        std::unique_lock<std::mutex> guard(mutex);
        int count = this->records.count();
        if (count == 0){
            mutex.unlock();
            sleep(1);
            continue;
        }

        int i = 0;
        while (i < count) {
            auto* act = const_cast<ARP_record *>(&this->records.at(i));
            if (act->getTimer() == 0) {
                this->records.removeAt(i);
                count = this->records.count();
            } else {
                act->setTimer(act->getTimer() - 1);
                i++;
            }
        }
        guard.unlock();
        sleep(1);
    }
}

void ARP_table::add_record(ARP_record *record) {
    int recordCount = this->records.count();
    for (int i = 0; i < recordCount; ++i) {
        auto* act = const_cast<ARP_record *>(&this->records.at(i));

        if (act->getIP() == record->getIP()) {
            if (act->getMAC() == record->getMAC()) {
                act->setTimer(this->timer);
            } else {
                act->setMAC(record->getMAC());
            }
            return;
        }
    }

    record->setTimer(this->timer);
    this->records.append(*record);
}

void ARP_table::print() {
    int recordCount = this->records.count();
    QStringList list;
    QString header = "    IP\t          MAC\t\tTIMER";
    list << header << "";

    for (int i = 0; i < recordCount; ++i) {
        ARP_record act = this->records.at(i);
        QString ip = QString::fromStdString(act.getIP().to_string());
        QString mac = QString::fromStdString(act.getMAC().to_string());
        QString timer = QString::number(act.getTimer());

        QString record = ip + "\t" + mac + "\t   " + timer;

        list << record;
    }

    emit printTable(list);
}

void ARP_table::set_timer(unsigned int value) {
    std::unique_lock<std::mutex> guard(mutex);
    this->timer = value;
    guard.unlock();
}

void ARP_table::set_stop(bool value) {
    this->stop = value;
}

void ARP_table::clear() {
    std::unique_lock<std::mutex> guard(mutex);
    this->records.clear();
    guard.unlock();
}

HWAddress<6>* ARP_table::findRecord(IPv4Address targetIP) {
    auto* targetMAC = new HWAddress<6>;

    int recordCount = this->records.count();
    for (int i = 0; i < recordCount; ++i) {
        ARP_record act = this->records.at(i);
        if (act.getIP() == targetIP){
            *targetMAC = act.getMAC();
            return targetMAC;
        }
    }

    return nullptr;
}

ARP_table::ARP_table(QObject *parent) : QThread(parent) {

}
