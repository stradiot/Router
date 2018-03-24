#include <iostream>
#include "RIPv2_database.h"


void RIPv2_database::run() {

}

void RIPv2_database::processRecord(RIPv2_record *record) {
    std::unique_lock<std::mutex> guard(mutex);

    int index = -1;
    int recordCount = records.count();

    if (record->metric == 16){
        solvePossiblyDown(record);
        return;
    }

    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);

        if (act->network == record->network && act->prefix_length == record->prefix_length && act->next_hop == record->next_hop){
            index = i;
            break;
        }
    }

    if (index == -1){
        records.append(record);
    } else {
        records[index]->metric = record->metric;
        records[index]->timer_invalid = this->timer_invalid;
        records[index]->timer_holddown = this->timer_holddown;
        records[index]->timer_flush = this->timer_flush;
    }

    findBestRoute(record);

    print();

    guard.unlock();
}


void RIPv2_database::solvePossiblyDown(RIPv2_record *record) {
    int index = -1;
    int recordCount = records.count();

    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);

        if (act->network == record->network && act->prefix_length == record->prefix_length && act->next_hop == record->next_hop){
            index = i;
            break;
        }
    }

    if (index == -1) {
        record->possibly_down = true;
        record->timer_invalid = 0;
        record->timer_holddown = this->timer_holddown;
        record->timer_flush = this->timer_flush;

        //send triggered update
        records.append(record);
    } else {
        records[index]->metric = 16;
        records[index]->possibly_down = true;
        records[index]->timer_invalid = 0;

        //send triggered update
        findBestRoute(record);
    }

    print();
}


void RIPv2_database::findBestRoute(RIPv2_record *record) {
    RIPv2_record* best = record;
    int recordCount = records.count();

    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);

        if (act->network == record->network && act->prefix_length == record->prefix_length && act->metric <= best->metric)
            best = act;
    }

    routing_table->deleteRecord(record->network, record->prefix_length, "R");

    if (best->metric == 16)
        return;

    auto *routing_table_record = new Routing_table_record;
    routing_table_record->network = best->network;
    routing_table_record->netmask = best->prefix_length;
    routing_table_record->nextHop = best->next_hop;
    routing_table_record->interface = best->interface;
    routing_table_record->administrativeDistance = 120;
    routing_table_record->protocol = "R";

    routing_table->addRecord(routing_table_record);
}


void RIPv2_database::print() {
    QStringList list;

    list << "update timer: " + QString::number(this->timer_update);

    int recordCount = records.count();
    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);

        QString network = QString::fromStdString(act->network.to_string());
        QString mask = QString::number(act->prefix_length);
        QString nextHop = QString::fromStdString(act->next_hop.to_string());
        QString interface = QString::fromStdString(act->interface);
        QString metric = QString::number(act->metric);
        QString possibly_down = QString::number(act->possibly_down);
        QString invalid = QString::number(act->timer_invalid);
        QString holddown = QString::number(act->timer_holddown);
        QString flush = QString::number(act->timer_flush);

        QString record = network + "/" + mask + " " + nextHop + " " + interface + " " + metric + " " + possibly_down + " ";
        record += invalid + " " + holddown + " " + flush;

        list << record;
    }

    emit print_database(list);
}

RIPv2_database::RIPv2_database(QObject *parent) : QThread(parent) {

}
