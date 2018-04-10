#ifndef ROUTER_ARP_RECORD_H
#define ROUTER_ARP_RECORD_H

#include <tins/tins.h>

using namespace Tins;

class ARP_record {
public:
    explicit ARP_record(IPv4Address ip, HWAddress<6> mac);
    IPv4Address ip;
    HWAddress<6> mac;
    unsigned int timer;

};


#endif
