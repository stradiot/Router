#include <iostream>
#include "Routing_table.h"

void Routing_table::addRecord(Routing_table_record *record) {
    for (unsigned long i = 0; i < records.size(); ++i) {
        auto act = records.at(i);
        if ((act->network.to_string() == record->network.to_string()) && (act->netmask == record->netmask)) {
            if (act->administrativeDistance > record->administrativeDistance) {
                records.erase(records.begin() + i);
                records.push_back(record);
            }
            print();
            return;
        }
    }

    records.push_back(record);
    print();
}

void Routing_table::print() {
    QStringList list;

    for (unsigned long i = 0; i < records.size(); ++i) {
        auto act = records.at(i);

        QString protocol = QString::fromStdString(act->protocol);
        QString network = QString::fromStdString(act->network.to_string());
        QString mask = QString::number(act->netmask);
        QString ad = QString::number(act->administrativeDistance);
        QString metric = QString::number(act->metric);
        QString nextHop = QString::fromStdString(act->nextHop.to_string());
        if (nextHop == "0.0.0.0")
            nextHop = "";
        QString interface = QString::fromStdString(act->interface);
        if (interface == "none")
            interface = "";

        QString record = QString::number(i) +  "\t     "  + protocol + "\t" + network + "/" + mask + "\t [" + ad + "/" + metric + "]\t" + nextHop + "\t" + interface;

        list << record;
    }

    emit printTable(list);
}

void Routing_table::clear() {
    records.clear();
    print();
}

Routing_table_record *Routing_table::findRecord(IPv4Address dest) {
    Routing_table_record *best = nullptr;

    for (auto act : records) {
        IPv4Range range = act->network / act->netmask;

        if (range.contains(dest) || (act->network == "0.0.0.0" && act->netmask == 0)){
            if (best == nullptr)
                best = act;
            else
                if (best->netmask < act->netmask)
                    best = act;
        }
    }

    return best;
}

void Routing_table::deleteRecord(unsigned long index) {
    auto *record = records.at(index);
    records.erase(records.begin() + index);
    emit findReplacement(record->network, record->netmask);
    print();
}

void Routing_table::deleteRecord(string protocol, string interface) {
    for (unsigned long i = 0; i < records.size(); ++i) {
        auto act = records.at(i);
        if ((act->protocol == protocol) && (act->interface == interface))
            records.erase(records.begin() + i--);
    }

    print();
}

vector<Routing_table_record> Routing_table::fillUpdate(string interface) {
    vector<Routing_table_record> vec;

    for (auto act: records) {
        if ((act->protocol == "C" || act->protocol == "R") && (act->interface == interface))
            vec.push_back(*act);
    }

    return vec;
}

void Routing_table::deleteRecord(IPv4Address network, unsigned int prefix_length, IPv4Address nexthop, string protocol) {
    for (unsigned long i = 0; i < records.size(); ++i) {
        auto act = records.at(i);
        if (act->protocol == protocol && act->network == network && act->netmask == prefix_length && act->nextHop == nexthop)
            records.erase(records.begin() + i--);
    }

    print();
}
