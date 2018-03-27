#include <iostream>
#include "Interface.h"
#include "RIPv2_record.h"
#include <QtWidgets/QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <iomanip>
#include <sstream>
#include <boost/asio/ip/address_v4.hpp>

void Interface::run() {
    while (true){
        PDU* pdu = sniffer->next_packet();

        if (this->use_RIPv2 == true){
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
        if (ip != nullptr){
            if (ip->dst_addr() == *this->ipv4){
                auto* udp = pdu->find_pdu<UDP>();
                if (udp != nullptr)
                    if (udp->dport() == 520)
                        processRIPv2(ip);
            } else {
                if (ip->ttl() > 1)
                    forwardPacket(ip);
            }
        }
    }
}


void Interface::processRIPv2(IP* ip) {
    auto *raw = ip->find_pdu<RawPDU>();

    stringstream command;
    stringstream version;
    command << hex << (int)(raw->payload().at(0));
    version << hex << (int)(raw->payload().at(1));

    if (version.str() != "2")
        return;

    if (command.str() == "2")
        processRIPv2Response(ip);
    else if (command.str() == "1"){
        auto *eth = ip->parent_pdu()->find_pdu<EthernetII>();
        if (eth != nullptr)
            processRIPv2Request(eth);
    }
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

        stringstream metric_string;

        for (int j = 0; j < 3; ++j) {
            metric_string << (int)(raw->payload().at(cursor++));
        }
        metric_string << (int)(raw->payload().at(cursor++));

        unsigned int metric = (unsigned int)stoi(metric_string.str());

        auto *rip_record = new RIPv2_record;
        rip_record->network = network_string.str();
        IPv4Address rip_prefix_length = netmask_string.str();
        rip_record->prefix_length = (unsigned int)calculate_prefix_length(rip_prefix_length);
        rip_record->interface = this->interface->name();
        if (nexthop_string.str() == "0.0.0.0")
            rip_record->next_hop = ip->src_addr();
        else
            rip_record->next_hop = nexthop_string.str();
        rip_record->metric = min(metric + 1, (unsigned int)16);
        rip_record->timer_invalid = ripv2_database->timer_invalid;
        rip_record->timer_holddown = ripv2_database->timer_holddown;
        rip_record->timer_flush = ripv2_database->timer_flush;

        ripv2_database->processRecord(rip_record);
    }
}


int Interface::calculate_prefix_length (uint32_t address) {
    int set_bits;
    for (set_bits = 0; address; address >>= 1) {
        set_bits += address & 1;
    }
    return set_bits;
}

Interface::Interface(ARP_table* arp_table, Routing_table* routing_table, RIPv2_database* ripv2_database) {
    this->config.set_promisc_mode(true);
    this->config.set_immediate_mode(true);
    this->config.set_direction(PCAP_D_IN);

    this->arp_table = arp_table;
    this->routing_table = routing_table;
    this->ripv2_database = ripv2_database;
}

void Interface::setIPv4(string interface, string ipv4, string mask) {
    delete this->ipv4;
    delete this->sniffer;

    this->ipv4 = new IPv4Address(ipv4);
    this->netmask = (unsigned int)std::stoi(mask);

    this->interface = new NetworkInterface(interface);

    this->sniffer = new Sniffer(interface, this->config);

    if (use_RIPv2)
        ripv2_database->sendRequest(interface);
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

void Interface::onUseRipv2(bool value) {
    this->use_RIPv2 = value;

    ripv2_database->sendRequest(this->interface->name());
}

void Interface::processRIPv2Request(EthernetII *eth_pdu) {
    IP *ip_pdu = eth_pdu->find_pdu<IP>();
    if (ip_pdu == nullptr)
        return;

    auto vec = routing_table->fillUpdate(otherInterface->interface->name());
    vector<RIPv2_record> vec_rec;

    for (auto rec : vec){
        RIPv2_record record;
        record.network = rec.network;
        record.prefix_length = rec.netmask;
        record.metric = rec.metric;

        vec_rec.push_back(record);
    }

    for (auto rec : ripv2_database->records) {
        if (rec->metric == 16 && rec->interface == otherInterface->interface->name())
            vec_rec.push_back(*rec);
    }

    while (!vec_rec.empty()){
        vector<uint8_t> payload{
                0x02, 0x02, 0x00,0x00
        };

        vector<uint8_t> header{
                0x00, 0x02, 0x00, 0x00
        };

        for (int k = 0; k < 25; ++k) {
            auto record = vec_rec.back();
            vec_rec.pop_back();

            IPv4Address network = record.network;
            array<uint8_t, 4> arr_net = boost::asio::ip::address_v4::from_string(network.to_string()).to_bytes();
            vector<uint8_t> vec_net(arr_net.begin(), arr_net.end());

            IPv4Address netmask = IPv4Address::from_prefix_length(record.prefix_length);
            array<uint8_t, 4> arr_mask = boost::asio::ip::address_v4::from_string(netmask.to_string()).to_bytes();
            vector<uint8_t> vec_mask(arr_mask.begin(), arr_mask.end());

            vector<uint8_t> vec_nh{
                    0x00, 0x00,  0x00, 0x00
            };

            int metric = min(record.metric, (unsigned int)16);
            vector<uint8_t> vec_metric;

            for (int j = 0; j < 4; ++j) {
                vec_metric.push_back(metric & 0xFF);
                metric >>= 8;
            }

            reverse(vec_metric.begin(), vec_metric.end());

            payload.insert(payload.end(), header.begin(), header.end());
            payload.insert(payload.end(), vec_net.begin(), vec_net.end());
            payload.insert(payload.end(), vec_mask.begin(), vec_mask.end());
            payload.insert(payload.end(), vec_nh.begin(), vec_nh.end());
            payload.insert(payload.end(), vec_metric.begin(), vec_metric.end());

            if (vec_rec.empty())
                break;
        }

        RawPDU raw(payload.begin(), payload.end());
        UDP udp(520, 520);
        IP ip(ip_pdu->src_addr(), ipv4->to_string());
        ip.ttl(255);
        EthernetII eth(eth_pdu->src_addr(), this->interface->hw_address());

        eth /= ip / udp / raw;

        PacketSender sender(*this->interface);
        sender.send(eth);
    }
}
