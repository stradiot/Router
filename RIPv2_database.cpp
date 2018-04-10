#include <iostream>
#include <boost/asio.hpp>
#include "RIPv2_database.h"


void RIPv2_database::run() {
    while (1){
        for (int i = 0; i < records.count(); ++i) {
            auto act = records.at(i);

            if (act->possibly_down){
                if (act->timer_flush == 0 || act->timer_holddown == 0){
                    routing_table->deleteRecord(act->network, act->prefix_length, act->next_hop, "R");
                    records.removeAt(i--);
                } else {
                    act->timer_holddown--;
                    act->timer_flush--;
                }
            } else {
                if (act->timer_invalid == 0){
                    act->possibly_down = true;
                    act->metric = 16;
                    routing_table->deleteRecord(act->network, act->prefix_length, act->next_hop, "R");

                    findBestRoute(act);

                    Routing_table_record record;
                    record.interface = act->interface;
                    record.network = act->network;
                    record.netmask = act->prefix_length;
                    record.metric = act->metric;
                    record.protocol = "R";
                    record.administrativeDistance = 120;
                    record.nextHop = act->next_hop;

                    routing_table->addRecord(&record);
                } else {
                    act->timer_invalid--;
                    act->timer_flush--;
                }
            }
        }

        update--;
        if (update == 0){
            sendUpdate(one->getInterface());
            sendUpdate(two->getInterface());
            update = timer_update;
        }
        print();
        sleep(1);
    }
}

void RIPv2_database::processRecord(RIPv2_record *record) {
    std::unique_lock<std::mutex> guard(mutex);
    IPv4Address connected_one = *one->ipv4 & IPv4Address::from_prefix_length(one->prefix_length);
    IPv4Address connected_two = *two->ipv4 & IPv4Address::from_prefix_length(two->prefix_length);

    if (record->network == connected_one || record->network == connected_two)
        return;

    if (record->metric == 16){
        solvePossiblyDown(record);
        return;
    }

    RIPv2_record* rec = nullptr;
    for (int i = 0; i < records.count(); ++i) {
        auto act = records.at(i);

        if (act->network == record->network && act->prefix_length == record->prefix_length && act->next_hop == record->next_hop){
            rec = act;
            break;
        }
    }

    if (rec == nullptr){
        records.append(record);
    } else {
        if (rec->possibly_down){
            print();
            guard.unlock();
            return;
        }
        rec->metric = record->metric;
        rec->timer_invalid = this->timer_invalid;
        rec->timer_flush = this->timer_flush;
    }

    findBestRoute(record);

    print();

    guard.unlock();
}

void RIPv2_database::solvePossiblyDown(RIPv2_record *record) {
    RIPv2_record* rec = nullptr;
    for (int i = 0; i < records.count(); ++i) {
        auto act = records.at(i);

        if (act->network == record->network && act->prefix_length == record->prefix_length && act->next_hop == record->next_hop){
            rec = act;
            break;
        }
    }

    if (rec == nullptr){
        record->possibly_down = true;
        record->timer_invalid = 0;
        record->timer_holddown = this->timer_holddown;
        record->timer_flush = this->timer_flush;

        sendTriggeredUpdate(record);
        records.append(record);
    } else if (!rec->possibly_down){
        rec->metric = 16;
        rec->possibly_down = true;
        rec->timer_invalid = 0;

        sendTriggeredUpdate(record);
        findBestRoute(record);
    }

    print();
}


void RIPv2_database::findBestRoute(RIPv2_record *record) {
    auto RIPv2record = routing_table->findRecord(record->network);

    if (RIPv2record != nullptr){
        if (record->metric < RIPv2record->metric || record->metric == 16){
            routing_table->deleteRecord(RIPv2record->network, RIPv2record->netmask, RIPv2record->nextHop, "R");
        }
    }

    if (record->metric == 16){
        for (auto rec : records){
            if (rec->network == record->network && rec->prefix_length == record->prefix_length && rec->metric < record->metric)
                record = rec;
        }

        if (record->metric == 16)
            return;
    }

    auto *routing_table_record = new Routing_table_record;
    routing_table_record->network = record->network;
    routing_table_record->netmask = record->prefix_length;
    routing_table_record->nextHop = record->next_hop;
    routing_table_record->interface = record->interface;
    routing_table_record->administrativeDistance = 120;
    routing_table_record->metric = record->metric;
    routing_table_record->protocol = "R";

    routing_table->addRecord(routing_table_record);
}

void RIPv2_database::print() {
    QStringList list;

    int recordCount = records.count();
    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);

        QString network = QString::fromStdString(act->network.to_string());
        QString mask = QString::number(act->prefix_length);
        QString nextHop = QString::fromStdString(act->next_hop.to_string());
        QString interface = QString::fromStdString(act->interface);
        QString metric = QString::number(act->metric);
        QString possibly_down = QString::number(act->possibly_down);
        QString invalid = QString::number(act->timer_invalid);
        QString holddown = QString::number(act->timer_holddown);
        QString flush = QString::number(act->timer_flush);

        QChar ch;
        if (possibly_down == "1")
             ch = 'P';
        else
            ch = ' ';

        QString record = network + "/" + mask + "\t" + nextHop + "\t" + interface + "\t" + metric + "\t" + ch + "\t";
        record += invalid + "\t" + holddown + "\t" + flush;

        list << record;
    }

    emit print_database(list, update);
}

RIPv2_database::RIPv2_database(QObject *parent) : QThread(parent) {

}

void RIPv2_database::sendTriggeredUpdate(RIPv2_record *record) {
    vector<uint8_t> payload{
            0x02, 0x02, 0x00,0x00, 0x00, 0x02, 0x00, 0x00
    };

    IPv4Address network = record->network;
    array<uint8_t, 4> arr_net = boost::asio::ip::address_v4::from_string(network.to_string()).to_bytes();
    vector<uint8_t> vec_net(arr_net.begin(), arr_net.end());

    IPv4Address netmask = IPv4Address::from_prefix_length(record->prefix_length);
    array<uint8_t, 4> arr_mask = boost::asio::ip::address_v4::from_string(netmask.to_string()).to_bytes();
    vector<uint8_t> vec_mask(arr_mask.begin(), arr_mask.end());

    vector<uint8_t> vec_nh{
            0x00, 0x00,  0x00, 0x00
    };

    vector<uint8_t> vec_metric{
            0x00, 0x00, 0x00, 0x10
    };

    payload.insert(payload.end(), vec_net.begin(), vec_net.end());
    payload.insert(payload.end(), vec_mask.begin(), vec_mask.end());
    payload.insert(payload.end(), vec_nh.begin(), vec_nh.end());
    payload.insert(payload.end(), vec_metric.begin(), vec_metric.end());

    RawPDU raw(payload.begin(), payload.end());
    UDP udp(520, 520);
    IP ip;
    ip.ttl(2);
    ip.dst_addr("224.0.0.9");
    EthernetII eth;
    HWAddress<6> add("01:00:5E:00:00:09");
    eth.dst_addr(add);

    if (record->interface == one->getInterface()){
        ip.src_addr(two->ipv4->to_string());

        eth /= ip / udp / raw;

        PacketSender sender(two->getInterface());
        sender.send(eth);
        two->count_statistics_OUT(&eth);
    } else {
        ip.src_addr(one->ipv4->to_string());

        eth /= ip / udp / raw;

        PacketSender sender(one->getInterface());
        sender.send(eth);
        one->count_statistics_OUT(&eth);
    }
}

void RIPv2_database::sendUpdate(string interface) {
    auto vec = routing_table->fillUpdate(interface);
    vector<RIPv2_record> vec_rec;

    for (auto rec : vec){
        RIPv2_record record;
        record.network = rec.network;
        record.prefix_length = rec.netmask;
        record.metric = rec.metric;

        vec_rec.push_back(record);
    }

    for (auto rec : records) {
        if (rec->metric == 16 && rec->interface == interface)
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
        IP ip;
        ip.ttl(2);
        ip.dst_addr("224.0.0.9");
        EthernetII eth;
        HWAddress<6> add("01:00:5E:00:00:09");
        eth.dst_addr(add);

        if (interface == one->getInterface()){
            if (!ripv2_two)
                return;
            ip.src_addr(two->ipv4->to_string());

            eth /= ip / udp / raw;

            PacketSender sender(two->getInterface());
            sender.send(eth);
            two->count_statistics_OUT(&eth);
        } else {
            if (!ripv2_one)
                return;
            ip.src_addr(one->ipv4->to_string());

            eth /= ip / udp / raw;

            PacketSender sender(one->getInterface());
            sender.send(eth);
            one->count_statistics_OUT(&eth);
        }
    }
}

void RIPv2_database::sendRequest(string interface) {
    vector<uint8_t> payload{
            0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10
    };

    RawPDU raw(payload.begin(), payload.end());
    UDP udp(520, 520);
    IP ip;
    ip.ttl(2);
    ip.dst_addr("224.0.0.9");
    EthernetII eth;
    HWAddress<6> add("01:00:5E:00:00:09");
    eth.dst_addr(add);

    if (interface == one->getInterface()){
        ip.src_addr(one->ipv4->to_string());

        eth /= ip / udp / raw;

        PacketSender sender(one->getInterface());
        sender.send(eth);
        one->count_statistics_OUT(&eth);
    } else {
        ip.src_addr(two->ipv4->to_string());

        eth /= ip / udp / raw;

        PacketSender sender(two->getInterface());
        sender.send(eth);
        two->count_statistics_OUT(&eth);
    }
}

void RIPv2_database::set_timerUpdate(int update) {
    this->timer_update = (unsigned int)update;
}

void RIPv2_database::set_timerInvalid(int invalid) {
    this->timer_invalid = (unsigned int)invalid;
}

void RIPv2_database::set_timerHolddown(int holddown) {
    this->timer_holddown = (unsigned int)holddown;
}

void RIPv2_database::set_timerFlush(int flush) {
    this->timer_flush = (unsigned int)flush;
}

void RIPv2_database::set_ripv2_one(bool use) {
    this->ripv2_one = use;
}

void RIPv2_database::set_ripv2_two(bool use) {
    this->ripv2_two = use;
}

void RIPv2_database::findReplacement(IPv4Address network, unsigned int prefix_length) {
    RIPv2_record* best = nullptr;

    for (int i = 0; i < records.count(); ++i) {
        auto act = records.at(i);

        if (act->network == network && act->prefix_length == prefix_length)
            if (best == nullptr || best->metric > act->metric)
                best = act;
    }

    if (best != nullptr){
        auto *record = new Routing_table_record;

        record->network = best->network;
        record->netmask = best->prefix_length;
        record->metric = best->metric;
        record->administrativeDistance = 120;
        record->nextHop = best->next_hop;
        record->protocol = "R";
        record->interface = best->interface;

        routing_table->addRecord(record);
    }
}
