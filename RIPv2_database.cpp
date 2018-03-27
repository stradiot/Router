#include <iostream>
#include <boost/asio.hpp>
#include "RIPv2_database.h"


void RIPv2_database::run() {
    while (1){
        if (update == 0){
            sendUpdate(interface_one);
            sendUpdate(interface_two);
            update = timer_update;
        }

        int recordCount = records.count();
        vector<int> delete_indexes;

        for (int i = 0; i < recordCount; ++i) {
            auto act = records.at(i);

            if (act->possibly_down){
                if (act->timer_flush == 0 || act->timer_holddown == 0){
                    routing_table->deleteRecord(act->network, act->prefix_length, "R");
                    delete_indexes.push_back(i);
                } else {
                    records[i]->timer_holddown--;
                    records[i]->timer_flush--;
                }
            } else {
                if (act->timer_invalid == 0){
                    records[i]->possibly_down = true;
                    records[i]->metric = 16;
                } else {
                    records[i]->timer_invalid--;
                    records[i]->timer_flush--;
                }
            }
        }

        if (!delete_indexes.empty()){
            for (auto i : delete_indexes)
                records.removeAt(i);
        }

        update--;
        print();
        sleep(1);
    }
}

void RIPv2_database::processRecord(RIPv2_record *record) {
    std::unique_lock<std::mutex> guard(mutex);

    int index = -1;
    int recordCount = records.count();

    if (record->network == connected_one || record->network == connected_two)
        return;

    if (record->metric == 16){
        solvePossiblyDown(record);
        return;
    }

    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);

        if (act->network == record->network && act->prefix_length == record->prefix_length && act->next_hop == record->next_hop){
            index = i;
            break;
        }
    }

    if (index == -1){
        records.append(record);
    } else {
        if (records[index]->possibly_down){
            print();
            guard.unlock();
            return;
        }
        records[index]->metric = record->metric;
        records[index]->timer_invalid = this->timer_invalid;
        records[index]->timer_flush = this->timer_flush;
    }

    findBestRoute(record);

    print();

    guard.unlock();
}

void RIPv2_database::solvePossiblyDown(RIPv2_record *record) {
    int index = -1;
    int recordCount = records.count();

    for (int i = 0; i < recordCount; ++i) {
        auto act = records.at(i);

        if (act->network == record->network && act->prefix_length == record->prefix_length && act->next_hop == record->next_hop){
            index = i;
            break;
        }
    }

    if (index == -1) {
        record->possibly_down = true;
        record->timer_invalid = 0;
        record->timer_holddown = this->timer_holddown;
        record->timer_flush = this->timer_flush;

        sendTriggeredUpdate(record);
        records.append(record);
    } else if (!records[index]->possibly_down){
        records[index]->metric = 16;
        records[index]->possibly_down = true;
        records[index]->timer_invalid = 0;

        sendTriggeredUpdate(record);
        findBestRoute(record);
    }

    print();
}


void RIPv2_database::findBestRoute(RIPv2_record *record) {
    auto rec = routing_table->findRecord(record->network);

    if (rec != nullptr){
        if (record->metric < rec->metric || record->metric == 16){
            routing_table->deleteRecord(rec->network, rec->netmask, "R");
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

    list << "update in: " + QString::number(update) << "";
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

    emit print_database(list);
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

    if (record->interface == interface_one){
        ip.src_addr(address_two);

        eth /= ip / udp / raw;

        PacketSender sender(interface_two);
        sender.send(eth);
    } else {
        ip.src_addr(address_one);

        eth /= ip / udp / raw;

        PacketSender sender(interface_one);
        sender.send(eth);
    }
}

void RIPv2_database::sendUpdate(string interface) {
    unique_lock<std::mutex> guard(mutex);

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

        if (interface == interface_one){
            ip.src_addr(address_two);

            eth /= ip / udp / raw;

            PacketSender sender(interface_two);
            sender.send(eth);
        } else {
            ip.src_addr(address_one);

            eth /= ip / udp / raw;

            PacketSender sender(interface_one);
            sender.send(eth);
        }
    }

    guard.unlock();
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

    if (interface == interface_one){
        ip.src_addr(address_one);

        eth /= ip / udp / raw;

        PacketSender sender(interface_one);
        sender.send(eth);
    } else {
        ip.src_addr(address_two);

        eth /= ip / udp / raw;

        PacketSender sender(interface_two);
        sender.send(eth);
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
