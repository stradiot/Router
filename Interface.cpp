#include <iostream>
#include "Interface.h"
#include <boost/asio.hpp>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFuture>
#include <QtConcurrent/QtConcurrent>
#include <thread>
#include <future>

void Interface::run() {
    while (true){
        PDU* pdu = sniffer->next_packet();

        auto* arp = pdu->find_pdu<ARP>();
        if (arp != nullptr){
            processARP(arp);
            continue;
        }

        auto* ip = pdu->find_pdu<IP>();
        if (ip == nullptr)
            continue;
        if ((ip->dst_addr() == this->ipv4->to_string()) || (ip->dst_addr() == otherInterface->ipv4->to_string()))
            processIP(ip);
        else
            forwardPacket(ip);
    }
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

string Interface::getInterface() {
    if (this->interface == nullptr)
        return "none";
    else
        return this->interface->name();
}

void Interface::sendARPreply(ARP* request) {
    EthernetII reply;

    reply = ARP::make_arp_reply(request->sender_ip_addr(), request->target_ip_addr(), request->sender_hw_addr(), interface->hw_address());

    PacketSender sender;
    sender.send(reply, *this->interface);
}

void Interface::sendARPrequest(IPv4Address targetIP) {
    EthernetII request;
    request = ARP::make_arp_request(targetIP, *this->ipv4, this->interface->hw_address());

    PacketSender sender;
    sender.send(request, *this->interface);
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

void Interface::setOtherInterface(Interface *interface) {
    this->otherInterface = interface;
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

void Interface::forwardPacket(IP* ip) {
    IPv4Address targetIP = ip->dst_addr();

    Routing_table_record* record = routing_table->findRecord(targetIP);
    if (record == nullptr)
        return;

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

void Interface::processIP(IP *ip) {
    auto* icmp = ip->find_pdu<ICMP>();

    if (icmp != nullptr)
        processPING(icmp);
}




