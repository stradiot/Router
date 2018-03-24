#ifndef ROUTER_ROUTING_TABLE_H
#define ROUTER_ROUTING_TABLE_H

#include <QtCore/QList>
#include <QtCore/QArgument>
#include "Routing_table_record.h"
#include <QObject>

class Routing_table : public QObject{
    Q_OBJECT
public:
    void addRecord(Routing_table_record* record);
    Routing_table_record* findRecord(IPv4Address dest);
    void deleteRecord(int index);
    void deleteRecord(std::string protocol, std::string interface);
    void deleteRecord(IPv4Address network, unsigned int prefix_length, std::string protocol);
    void clear();
    void print();
private:
    QList<Routing_table_record> records;

signals:
    void printTable(QStringList list);



};


#endif
