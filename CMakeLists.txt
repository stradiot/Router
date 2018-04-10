cmake_minimum_required(VERSION 3.9)
project(Router)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_FLAGS  "-pthread")
set(CMAKE_CXX_FLAGS  "-lpcap")

find_package(Boost 1.62 COMPONENTS system REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)

add_executable(Router main.cpp Interface.cpp Interface.h Gui.cpp Gui.h ui_Gui.h ARP_table.cpp ARP_table.h ARP_record.h ARP_record.cpp Routing_table.cpp Routing_table.h Routing_table_record.h RIPv2_database.cpp RIPv2_database.h RIPv2_record.h Statistics.h)

target_link_libraries(Router tins ${Boost_LIBRARIES} Qt5::Core Qt5::Widgets Qt5::Gui pcap)