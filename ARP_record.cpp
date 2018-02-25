#include "ARP_record.h"

ARP_record::ARP_record(IPv4Address ip, HWAddress<6> mac) {
    this->ip = ip;
    this->mac = mac;
}

void ARP_record::setTimer(unsigned int timer) {
    this->timer = timer;
}

IPv4Address ARP_record::getIP() const {
    return this->ip;
}

HWAddress<6> ARP_record::getMAC() const {
    return this->mac;
}

unsigned int ARP_record::getTimer() const {
    return this->timer;
}

void ARP_record::setMAC(HWAddress<6> mac) {
    this->mac = mac;
}
