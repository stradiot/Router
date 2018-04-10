#include "ARP_record.h"

ARP_record::ARP_record(IPv4Address ip, HWAddress<6> mac) {
    this->ip = ip;
    this->mac = mac;
}
