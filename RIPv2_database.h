#ifndef ROUTER_RIPV2_DATABASE_H
#define ROUTER_RIPV2_DATABASE_H

#include <QtCore/QThread>
#include <mutex>
#include "RIPv2_record.h"
#include "Routing_table.h"
#include "Interface.h"


class Interface;
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

    bool ripv2_one;
    bool ripv2_two;

    Interface* one;
    Interface* two;

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
    void print_database(QStringList list, unsigned int update_in);

public slots:
    void findReplacement(IPv4Address network, unsigned int prefix_length);
    void set_ripv2_one(bool use);
    void set_ripv2_two(bool use);
    void set_timerUpdate(int update);
    void set_timerInvalid(int invalid);
    void set_timerHolddown(int holddown);
    void set_timerFlush(int flush);
};


#endif
