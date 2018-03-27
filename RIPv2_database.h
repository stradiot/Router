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
    unsigned int update = timer_update;
    unsigned int timer_invalid = 180;
    unsigned int timer_holddown = 180;
    unsigned int timer_flush = 240;

    Routing_table* routing_table = nullptr;
    IPv4Address connected_one;
    IPv4Address connected_two;
    IPv4Address address_one;
    IPv4Address address_two;
    std::string interface_one;
    std::string interface_two;

    QList<RIPv2_record*> records;

    void run() override;
    void processRecord(RIPv2_record *record);
    void solvePossiblyDown(RIPv2_record* record);
    void findBestRoute(RIPv2_record *record);
    void sendRequest(string interface);
    void sendUpdate(string interface);
    void sendTriggeredUpdate(RIPv2_record* record);
    void print();


private:
    std::mutex mutex;

signals:
    void print_database(QStringList list);

public slots:
    void set_timerUpdate(int update);
    void set_timerInvalid(int invalid);
    void set_timerHolddown(int holddown);
    void set_timerFlush(int flush);

};


#endif
