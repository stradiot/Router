#ifndef ROUTER_ROUTING_TABLE_RECORD_H
#define ROUTER_ROUTING_TABLE_RECORD_H

#include <tins/tins.h>

using namespace Tins;
using namespace std;

class Routing_table_record {
public:
    string protocol;
    IPv4Address network;
    unsigned int netmask;
    unsigned int administrativeDistance;
    unsigned int metric;
    string interface;
    IPv4Address nextHop;
};


#endif
