#ifndef ROUTER_ARP_RECORD_H
#define ROUTER_ARP_RECORD_H

#include <tins/tins.h>

using namespace Tins;

class ARP_record {
public:
    explicit ARP_record(IPv4Address ip, HWAddress<6> mac);
    void setTimer(unsigned int timer);
    void setMAC(HWAddress<6> mac);

    IPv4Address getIP() const;
    HWAddress<6> getMAC() const ;
    unsigned int getTimer() const ;
private:
    IPv4Address ip;
    HWAddress<6> mac;
    unsigned int timer;

};


#endif
