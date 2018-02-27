#ifndef ROUTER_INTERFACE_H
#define ROUTER_INTERFACE_H

#include <tins/tins.h>
#include "ARP_table.h"

using namespace Tins;
using namespace std;

class Interface : public QThread{
    Q_OBJECT

public:
    explicit Interface(ARP_table* arp_table);
    void setIPv4(string interface, string ipv4, string mask);
    void run() override;
    void setStop(bool value);
    string getInterface();

    void sendARPrequest(IPv4Address targetIP);
    void sendPING(IPv4Address targetIP);

private:
    NetworkInterface* interface;
    IPv4Address* ipv4 = new IPv4Address;
    IPv4Address* mask = new IPv4Address;
    IPv4Address* broadcast = new IPv4Address;

    ARP_table* arp_table;

    bool stop = true;
    SnifferConfiguration config;
    Sniffer* sniffer = new Sniffer("lo");

    QMutex mutex;

    void calculateBroadcast();
    void processPDU(PDU* pdu);
    void sendARPreply(ARP* request);
    void processARP(ARP* arp);
    void processPING(ICMP* icmp);
    void sendPINGreply(PDU* pdu);
};


#endif
