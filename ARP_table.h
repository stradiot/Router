#ifndef ROUTER_ARP_TABLE_H
#define ROUTER_ARP_TABLE_H

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <mutex>
#include "ARP_record.h"

using namespace std;

class ARP_table : public QThread{
    Q_OBJECT

public:
    explicit ARP_table(QObject *parent = nullptr);

    void add_record(ARP_record* record);
    void run() override;
    void print();
    void set_timer(unsigned int value);
    void set_stop(bool value);
    HWAddress<6>* findRecord(IPv4Address targetIP);
    void clear();

private:
    vector<ARP_record*> records;
    bool stop = false;
    unsigned int timer = 20;
    std::mutex mutex;

signals:
    void printTable(QStringList list);
};


#endif
