#ifndef ROUTER_INTERFACE_H
#define ROUTER_INTERFACE_H

#include <tins/tins.h>
#include "ARP_table.h"
#include "Routing_table.h"
#include "RIPv2_database.h"

using namespace Tins;
using namespace std;

class Interface : public QThread{
    Q_OBJECT

public:
    IPv4Address* ipv4  = nullptr;
    unsigned int netmask;

    explicit Interface(ARP_table* arp_table, Routing_table* routing_table, RIPv2_database* ripv2_database);
    void setIPv4(string interface, string ipv4, string mask, bool use_RIPv2);
    void setOtherInterface(Interface* interface);

    string getInterface();

    void run() override;

    void sendARPrequest(IPv4Address targetIP);
    void sendPINGrequest(IPv4Address targetIP);
    void sendPINGrequest(IPv4Address targetIP, IPv4Address* nextHop);
    void sendPacket(IP* ip, IPv4Address* nextHop);

private:
    NetworkInterface* interface = nullptr;

    Routing_table* routing_table  = nullptr;
    ARP_table* arp_table  = nullptr;

    SnifferConfiguration config;
    Sniffer* sniffer  = nullptr;

    Interface* otherInterface  = nullptr;

    bool use_RIPv2 = false;
    RIPv2_database* ripv2_database = nullptr;

    int calculate_prefix_length (uint32_t address);
    void sendARPreply(ARP* request);
    void processARP(ARP* arp);
    void processPING(ICMP* icmp);
    void sendPINGreply(PDU* pdu);
    void forwardPacket(IP* ip);
    void processRIPv2(IP* ip);
    void processRIPv2Response(IP* ip);
};


#endif
