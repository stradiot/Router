#ifndef ROUTER_ROUTING_TABLE_H
#define ROUTER_ROUTING_TABLE_H

#include <QtCore/QList>
#include <QtCore/QArgument>
#include "Routing_table_record.h"
#include <QObject>

using namespace std;

class Routing_table : public QObject{
    Q_OBJECT
public:
    void addRecord(Routing_table_record* record);
    Routing_table_record* findRecord(IPv4Address dest);
    vector<Routing_table_record> fillUpdate(string interface);
    void deleteRecord(unsigned long index);
    void deleteRecord(string protocol, string interface);
    void deleteRecord(IPv4Address network, unsigned int prefix_length, IPv4Address nexthop, string protocol);
    void clear();
    void print();
private:
    vector<Routing_table_record*> records;

signals:
    void printTable(QStringList list);
    void findReplacement(IPv4Address network, unsigned int prefix_length);
};


#endif
