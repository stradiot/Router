#ifndef ROUTER_STATISTICS_H
#define ROUTER_STATISTICS_H


class Statistics {

public:
    unsigned int EthernetII = 0;
    unsigned int ARP_REQ = 0;
    unsigned int ARP_REP = 0;
    unsigned int IP = 0;
    unsigned int ICMP_REQ = 0;
    unsigned int ICMP_REP = 0;
    unsigned int UDP = 0;
    unsigned int RIPv2_REQ = 0;
    unsigned int RIPv2_REP = 0;
};


#endif
