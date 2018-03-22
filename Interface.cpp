#include <iostream>
#include "Interface.h"
#include <QtWidgets/QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <iomanip>
#include <sstream>

void Interface::run() {
    while (true){
        PDU* pdu = sniffer->next_packet();

        auto* udp = pdu->find_pdu<UDP>();
        if (udp != nullptr) {
            if (udp->dport() == 520) {
                auto *ip = pdu->find_pdu<IP>();
                if (ip != nullptr) {
                    if (ip->dst_addr() == "224.0.0.9"){
                        processRIPv2(ip);
                        continue;
                    }
                }
            }
        }

        auto* arp = pdu->find_pdu<ARP>();
        if (arp != nullptr){
            processARP(arp);
            continue;
        }

        auto* icmp = pdu->find_pdu<ICMP>();
        if (icmp != nullptr){
            auto* ip = pdu->find_pdu<IP>();
            if (ip != nullptr){
                if ((ip->dst_addr() == this->ipv4->to_string()) || (ip->dst_addr() == otherInterface->ipv4->to_string())){
                    processPING(icmp);
                    continue;
                }
            }
        }
        auto* ip = pdu->find_pdu<IP>();
        if (ip != nullptr)
            if (ip->ttl() > 1)
                forwardPacket(ip);
    }
}


void Interface::processRIPv2(IP* ip) {
    auto *raw = ip->find_pdu<RawPDU>();

    unsigned long payloadSize = raw->payload_size();

    stringstream command;
    stringstream version;
    command << hex << (int)(raw->payload().at(0));
    version << hex << (int)(raw->payload().at(1));

    if (version.str() != "2")
        return;

    if (command.str() == "2")
        processRIPv2Response(ip);


}

void Interface::processRIPv2Response(IP *ip) {
    auto *raw = ip->find_pdu<RawPDU>();
    unsigned long payloadSize = raw->payload_size();
    unsigned long network_count = (payloadSize - 4) / 20;

    for (int i = 0; i < network_count; ++i) {
        int cursor = 4 + i * 20 + 4;

        stringstream network_string;

        for (int j = 0; j < 3; ++j) {
            network_string << (int)(raw->payload().at(cursor++)) <<  ".";
        }
        network_string << (int)(raw->payload().at(cursor++));

        stringstream netmask_string;

        for (int j = 0; j < 3; ++j) {
            netmask_string << (int)(raw->payload().at(cursor++)) <<  ".";
        }
        netmask_string << (int)(raw->payload().at(cursor++));

        stringstream nexthop_string;

        for (int j = 0; j < 3; ++j) {
            nexthop_string << (int)(raw->payload().at(cursor++)) <<  ".";
        }
        nexthop_string << (int)(raw->payload().at(cursor++));

        auto *record = new Routing_table_record;
        record->network = network_string.str();
        IPv4Address netmask(netmask_string.str());
        record->netmask = (unsigned int)calculate_prefix_length(netmask);
        if (nexthop_string.str() == "0.0.0.0")
            record->nextHop = ip->src_addr();
        else
            record->nextHop = nexthop_string.str();
        record->protocol = "R";
        record->administrativeDistance = 120;
        record->interface = this->interface->name();

        routing_table->addRecord(record);
    }
}


int Interface::calculate_prefix_length (uint32_t address) {
    int set_bits;
    for (set_bits = 0; address; address >>= 1) {
        set_bits += address & 1;
    }
    return set_bits;
}

Interface::Interface(ARP_table* arp_table, Routing_table* routing_table) {
    this->config.set_promisc_mode(true);
    this->config.set_immediate_mode(true);
    this->config.set_direction(PCAP_D_IN);

    this->arp_table = arp_table;
    this->routing_table = routing_table;
}

void Interface::setIPv4(string interface, string ipv4, string mask) {
    delete this->ipv4;
    delete this->sniffer;

    this->ipv4 = new IPv4Address(ipv4);
    this->netmask = (unsigned int)std::stoi(mask);

    this->interface = new NetworkInterface(interface);

    this->sniffer = new Sniffer(interface, this->config);
}

void Interface::setOtherInterface(Interface *interface) {
    this->otherInterface = interface;
}

string Interface::getInterface() {
    if (this->interface == nullptr)
        return "none";
    else
        return this->interface->name();
}


void Interface::sendARPrequest(IPv4Address targetIP) {
    EthernetII request;
    request = ARP::make_arp_request(targetIP, *this->ipv4, this->interface->hw_address());

    PacketSender sender;
    sender.send(request, *this->interface);
}

void Interface::sendARPreply(ARP* request) {
    EthernetII reply;

    reply = ARP::make_arp_reply(request->sender_ip_addr(), request->target_ip_addr(), request->sender_hw_addr(), interface->hw_address());

    PacketSender sender;
    sender.send(reply, *this->interface);
}

void Interface::processARP(ARP *arp) {
    if (arp == nullptr)
        return;

    Routing_table_record* record = routing_table->findRecord(arp->target_ip_addr());

    if (arp->opcode() == ARP::REQUEST){
        if ((record != nullptr) && (record->interface == otherInterface->interface->name())){
            sendARPreply(arp);
            return;
        } else if (arp->target_ip_addr() == this->ipv4->to_string()) {
            sendARPreply(arp);
            return;
        }
    }

    if (arp->opcode() == ARP::REPLY){
        if ((arp->target_ip_addr() == this->ipv4->to_string())){
            auto* arp_record = new ARP_record(arp->sender_ip_addr(), arp->sender_hw_addr());
            arp_table->add_record(arp_record);
        }
        return;
    }
}

void Interface::sendPINGrequest(IPv4Address targetIP) {
    auto* echo = new ICMP;
    echo->type(ICMP::ECHO_REQUEST);

    auto* ip = new IP(targetIP, *this->ipv4);
    *ip /= *echo;

    try {
        QtConcurrent::run(this, &Interface::sendPacket, ip, nullptr);
    }
    catch (...){}
}

void Interface::sendPINGrequest(IPv4Address targetIP, IPv4Address* nextHop) {
    auto* echo = new ICMP;
    echo->type(ICMP::ECHO_REQUEST);

    auto* ip = new IP(targetIP, *this->ipv4);
    *ip /= *echo;

    try {
        QtConcurrent::run(this, &Interface::sendPacket, ip, nextHop);
    }
    catch (...){}
}

void Interface::sendPINGreply(PDU *pdu) {
    auto* reply = pdu->find_pdu<ICMP>()->clone();
    reply->type(ICMP::ECHO_REPLY);

    auto* ip_request = pdu->find_pdu<IP>();
    auto* ip_reply = new IP(ip_request->src_addr(), ip_request->dst_addr());

    *ip_reply /= *reply;

    try {
        QtConcurrent::run(this, &Interface::sendPacket, ip_reply, nullptr);
    }
    catch (...){}

}

void Interface::processPING(ICMP *icmp) {
    if (icmp == nullptr)
        return;

    PDU* pdu = icmp->parent_pdu()->parent_pdu();
    auto* ip = pdu->find_pdu<IP>();

    if (ip == nullptr)
        return;

    if ((ip->dst_addr() == this->ipv4->to_string()) || (ip->dst_addr() == otherInterface->ipv4->to_string())){
        if (icmp->type() == ICMP::ECHO_REPLY){
            QMessageBox messageBox;
            messageBox.information(nullptr, "PING", "Ping successful");
        } else if (icmp->type() == ICMP::ECHO_REQUEST){
            sendPINGreply(pdu);
        }
    }
}

void Interface::sendPacket(IP* ip, IPv4Address* nextHop) {
    PacketSender sender;
    IPv4Address dest;
    if (nextHop == nullptr)
        dest = ip->dst_addr();
    else
        dest = *nextHop;

    HWAddress<6> *targetMAC = this->arp_table->findRecord(dest);

    for (int i = 0; i < 3; ++i) {
        if (targetMAC == nullptr){
            sendARPrequest(dest);
            sleep(1);
            targetMAC = this->arp_table->findRecord(dest);
        } else
            break;
    }

    if (targetMAC == nullptr){
        return;
    }

    EthernetII eth(*targetMAC, this->interface->hw_address());
    eth = eth / *ip;

    eth.send(sender, *this->interface);
}

void Interface::forwardPacket(IP* ip) {
    IPv4Address targetIP = ip->dst_addr();

    Routing_table_record* record = routing_table->findRecord(targetIP);
    if (record == nullptr)
        return;

    ip->ttl(ip->ttl() - (uint8_t)1);

    try {
        if (record->nextHop == "0.0.0.0")
            if (record->interface == this->interface->name()){
                return;
            } else {
                QtConcurrent::run(otherInterface, &Interface::sendPacket, ip, nullptr);
                return;
            }

        if (record->interface == "none"){
            IPv4Address* nextHop = &record->nextHop;

            while (record->interface == "none"){
                record = routing_table->findRecord(record->nextHop);
                if (record == nullptr)
                    return;
                if (record->interface != "none"){
                    if (record->nextHop != "0.0.0.0")
                        nextHop = &record->nextHop;
                    break;
                }

                nextHop = &record->nextHop;
            }

            if (record->interface == this->interface->name()){
                return;
            } else {
                QtConcurrent::run(otherInterface, &Interface::sendPacket, ip, nextHop);
                return;
            }
        }

        if (record->interface == this->getInterface()){
            return;
        } else {
            QtConcurrent::run(otherInterface, &Interface::sendPacket, ip, &record->nextHop);
            return;
        }
    }
    catch (...){}



}