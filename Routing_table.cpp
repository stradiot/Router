#include <iostream>
#include "Routing_table.h"

void Routing_table::addRecord(Routing_table_record *record) {
    int index = -1;

    int recordCount = records.count();
    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);
        if ((act.network.to_string() == record->network.to_string()) && (act.netmask == record->netmask)){
            index = i;
        }
    }

    if (index == -1)
        this->records.append(*record);
    else {
        auto old = records.at(index);
        if (old.administrativeDistance > record->administrativeDistance) {
            records.removeAt(index);
            records.append(*record);
        }
    }

    print();
}

void Routing_table::print() {
    QStringList list;

    int recordCount = records.count();
    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);

        QString protocol = QString::fromStdString(act.protocol);
        QString network = QString::fromStdString(act.network.to_string());
        QString mask = QString::number(act.netmask);
        QString ad = QString::number(act.administrativeDistance);
        QString nextHop = QString::fromStdString(act.nextHop.to_string());
        if (nextHop == "0.0.0.0")
            nextHop = "";
        QString interface = QString::fromStdString(act.interface);
        if (interface == "none")
            interface = "";

        QString record = QString::number(i) +  "\t     "  + protocol + "\t" + network + "/" + mask + "\t " + ad + "\t" + nextHop + "\t" + interface;

        list << record;
    }

    emit printTable(list);
}

void Routing_table::clear() {
    records.clear();
    print();
}

Routing_table_record *Routing_table::findRecord(IPv4Address dest) {
    int index = -1;

    int recordCount = records.count();
    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);
        IPv4Range range = act.network / act.netmask;

        if (range.contains(dest) || ((act.network == "0.0.0.0") && (act.netmask == 0))){
            if (index == -1)
                index = i;
            else
                if (records.at(index).netmask < act.netmask)
                    index = i;
        }
    }

    if (index == -1)
        return nullptr;
    else
        return (Routing_table_record *)&records.at(index);
}

void Routing_table::deleteRecord(int index) {
    records.removeAt(index);
    print();
}

void Routing_table::deleteRecord(std::string protocol, std::string interface) {
    std::vector<int> indexes;

    int recordCount = records.count();
    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);
        if ((act.protocol == protocol) && (act.interface == interface))
            indexes.push_back(i);
    }

    for (int i : indexes)
        records.removeAt(i);
}

void Routing_table::deleteRecord(IPv4Address network, unsigned int prefix_length, std::string protocol) {
    int index = -1;
    int recordCount = records.count();

    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);
        if (act.protocol == protocol && act.network == network && act.netmask == prefix_length)
            index = i;
    }

    if (index != -1)
        records.removeAt(index);
    print();
}
