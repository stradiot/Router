#ifndef ROUTER_RIPV2_DATABASE_H
#define ROUTER_RIPV2_DATABASE_H

#include <QtCore/QThread>
#include <mutex>
#include "RIPv2_record.h"
#include "Routing_table.h"

class RIPv2_database : public QThread{
    Q_OBJECT
public:
    explicit RIPv2_database(QObject *parent = nullptr);

    unsigned int timer_update = 30;
    unsigned int timer_invalid = 180;
    unsigned int timer_holddown = 180;
    unsigned int timer_flush = 240;

    Routing_table* routing_table = nullptr;

    void run() override;
    void processRecord(RIPv2_record *record);
    void solvePossiblyDown(RIPv2_record* record);
    void findBestRoute(RIPv2_record *record);
    void print();


private:
    QList<RIPv2_record*> records;
    std::mutex mutex;

signals:
    void print_database(QStringList list);

};


#endif
