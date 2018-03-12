#ifndef ROUTER_ROUTING_TABLE_RECORD_H
#define ROUTER_ROUTING_TABLE_RECORD_H

#include <tins/tins.h>

using namespace Tins;

class Routing_table_record {
public:
    std::string protocol;
    IPv4Address network;
    unsigned int netmask;
    unsigned int administrativeDistance;
    std::string interface;
    IPv4Address nextHop;
};


#endif
