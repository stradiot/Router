#include <iostream>
#include "Interface.h"
#include <boost/asio.hpp>
#include <QtWidgets/QMessageBox>

void Interface::run() {
    while (true){
        this->mutex.lock();
        if (this->stop){
            this->mutex.unlock();
            break;
        }
        PDU *pdu = (*sniffer).next_packet();
        processPDU(pdu);
        this->mutex.unlock();
    }
}

Interface::Interface(ARP_table* arp_table) {
    this->config.set_promisc_mode(true);
    this->config.set_immediate_mode(true);
    this->config.set_direction(PCAP_D_IN);

    this->arp_table = arp_table;
}

void Interface::setIPv4(string interface, string ipv4, string mask) {
    mutex.lock();
    delete this->ipv4;
    delete this->mask;
    delete this->sniffer;

    this->ipv4 = new IPv4Address(ipv4);
    this->mask = new IPv4Address(mask);

    this->calculateBroadcast();

    this->interface = new NetworkInterface(interface);

    this->sniffer = new Sniffer(interface, this->config);
    mutex.unlock();
}

void Interface::calculateBroadcast() {
    typedef boost::asio::ip::address_v4 Ip;
    Ip broadcast = Ip::broadcast(Ip::from_string(ipv4->to_string()), Ip::from_string(mask->to_string()));

    delete this->broadcast;
    this->broadcast = new IPv4Address(broadcast.to_string());
}

void Interface::setStop(bool value) {
    this->stop = value;
}

string Interface::getInterface() {
    return this->interface->name();
}

void Interface::processPDU(PDU *pdu) {
    pdu = pdu->inner_pdu();
    auto* arp = pdu->find_pdu<ARP>();
    auto*  icmp = pdu->find_pdu<ICMP>();

    processARP(arp);
    processPING(icmp);
}

void Interface::sendARPreply(ARP* request) {
    EthernetII reply;
    reply = ARP::make_arp_reply(request->sender_ip_addr(), request->target_ip_addr(), request->sender_hw_addr(), this->interface->hw_address());

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

    if (arp->target_ip_addr().to_string() == this->ipv4->to_string()){
        if (arp->opcode() == ARP::REPLY){
             ARP_record* record = new ARP_record(arp->sender_ip_addr(), arp->sender_hw_addr());
             this->arp_table->add_record(record);
        } else if (arp->opcode() == ARP::REQUEST){
            sendARPreply(arp);
        }
    }
}

void Interface::sendPING(IPv4Address targetIP) {
    PacketSender sender;

    ICMP echo;
    echo.type(ICMP::ECHO_REQUEST);

    IP ip(targetIP, *this->ipv4);

    HWAddress<6> *targetMAC = this->arp_table->findRecord(targetIP);

    for (int i = 0; i < 5; ++i) {
        if (targetMAC == nullptr){
            sendARPrequest(targetIP);
            sleep(1);
            targetMAC = this->arp_table->findRecord(targetIP);
        } else
            break;
    }

    if (targetMAC == nullptr){
        QMessageBox messageBox;
        messageBox.critical(nullptr,"Error","Destination unreachable\t");
        return;
    }

    EthernetII eth(*targetMAC, this->interface->hw_address());
    eth = eth / ip / echo;

    eth.send(sender, *this->interface);
}

void Interface::processPING(ICMP *icmp) {
    if (icmp == nullptr)
        return;

    PDU* pdu = icmp->parent_pdu()->parent_pdu();
    IP* ip = pdu->find_pdu<IP>();

    if (ip == nullptr)
        return;

    if (ip->dst_addr() == this->ipv4->to_string()){
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

    auto* eth_request = pdu->find_pdu<EthernetII>();
    auto* eth_reply = new EthernetII(eth_request->src_addr(), this->interface->hw_address());

    *eth_reply = *eth_reply / *ip_reply / *reply;

    PacketSender sender;
    sender.send(*eth_reply, *this->interface);
}




