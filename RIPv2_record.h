#ifndef ROUTER_RIPV2_RECORD_H
#define ROUTER_RIPV2_RECORD_H

#include <tins/tins.h>

using namespace std;
using namespace Tins;

class RIPv2_record {
public:
    IPv4Address network;
    unsigned int prefix_length;
    IPv4Address next_hop;
    string interface;
    unsigned int metric;

    unsigned int timer_invalid = 180;
    unsigned int timer_holddown = 180;
    unsigned int timer_flush = 240;

    bool possibly_down = false;
};


#endif //ROUTER_RIPV2_RECORD_H
