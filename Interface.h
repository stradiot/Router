#ifndef ROUTER_INTERFACE_H
#define ROUTER_INTERFACE_H

#include <tins/tins.h>
#include "ARP_table.h"
#include "Routing_table.h"

using namespace Tins;
using namespace std;

class Interface : public QThread{
    Q_OBJECT

public:
    explicit Interface(ARP_table* arp_table, Routing_table* routing_table);
    void setIPv4(string interface, string ipv4, string mask);
    void setOtherInterface(Interface* interface);

    string getInterface();

    void run() override;

    void sendARPrequest(IPv4Address targetIP);
    void sendPINGrequest(IPv4Address targetIP);
    void sendPINGrequest(IPv4Address targetIP, IPv4Address* nextHop);
    void sendPacket(IP* ip, IPv4Address* nextHop);

private:
    NetworkInterface* interface = nullptr;

    IPv4Address* ipv4  = nullptr;
    unsigned int netmask;

    Routing_table* routing_table  = nullptr;
    ARP_table* arp_table  = nullptr;

    SnifferConfiguration config;
    Sniffer* sniffer  = nullptr;

    Interface* otherInterface  = nullptr;

    void sendARPreply(ARP* request);
    void processARP(ARP* arp);
    void processPING(ICMP* icmp);
    void sendPINGreply(PDU* pdu);
    void forwardPacket(IP* ip);
    void processRIPv2(UDP* udp);
};


#endif
