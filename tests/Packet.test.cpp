#include <catch2/catch_test_macros.hpp>

#include <Packet.h>

using namespace DAQCap;

const int    PACKET_NUMBER_OVERFLOW = 65535;
const size_t PRELOAD_BYTES          = 14   ;
const size_t POSTLOAD_BYTES         = 4    ;

TEST_CASE("Packet constructor", "[Packet]") {

    SECTION("Packet constructor throws an exception if the size is too small") {

        unsigned char data[PRELOAD_BYTES + POSTLOAD_BYTES - 1];

        REQUIRE_THROWS_AS(Packet(data, PRELOAD_BYTES + POSTLOAD_BYTES - 1), std::invalid_argument);

    }

    SECTION("Packet constructor does not throw an exception if the size is sufficiently large") {

        unsigned char data[PRELOAD_BYTES + POSTLOAD_BYTES];

        REQUIRE_NOTHROW(Packet(data, PRELOAD_BYTES + POSTLOAD_BYTES));

    }

}   

TEST_CASE("Packet::size()", "[Packet]") {

    SECTION("Packet::size() returns 0 for an empty packet") {

        size_t packetSize = 0;
        size_t size = PRELOAD_BYTES + POSTLOAD_BYTES + packetSize;

        unsigned char data[size];
        Packet packet(data, size);

        REQUIRE(packet.size() == packetSize);

    }

    SECTION("Packet::size() returns the size of the data portion of the packet") {

        size_t packetSize = 10;
        size_t size = PRELOAD_BYTES + POSTLOAD_BYTES + packetSize;

        unsigned char data[size];
        Packet packet(data, size);

        REQUIRE(packet.size() == packetSize);

    }
    

}

TEST_CASE("Packet::getPacketNumber()", "[Packet]") {

    SECTION("Packet number does not rely on all but last two postload bytes") {

        size_t packetSize = 10;
        size_t size = PRELOAD_BYTES + POSTLOAD_BYTES + packetSize;

        unsigned char data[size];
        for(int i = 0; i < PRELOAD_BYTES; ++i) {
            data[i] = 0;
        }
        for(int i = PRELOAD_BYTES; i < PRELOAD_BYTES + packetSize; ++i) {
            data[i] = i - PRELOAD_BYTES + 1;
        }
        for(int i = size; i --> size - POSTLOAD_BYTES; i) {
            data[i] = 0;
        }
        for(int i = size - POSTLOAD_BYTES; i < size - 2; ++i) {
            data[i] = 0x01;
        }
        Packet packet(data, size);

        REQUIRE(packet.getPacketNumber() == 0);    

    }

    SECTION("Packet number is extracted correctly") {

        size_t packetSize = 10;
        size_t size = PRELOAD_BYTES + POSTLOAD_BYTES + packetSize;

        unsigned char data[size];
        for(int i = 0; i < PRELOAD_BYTES; ++i) {
            data[i] = 0;
        }
        for(int i = PRELOAD_BYTES; i < PRELOAD_BYTES + packetSize; ++i) {
            data[i] = i - PRELOAD_BYTES + 1;
        }
        for(int i = size; i --> size - POSTLOAD_BYTES; i) {
            data[i] = 0;
        }
        data[size - 2] = 0x01;
        data[size - 1] = 0x02;
        Packet packet(data, size);

        REQUIRE(packet.getPacketNumber() == 0x0102);

    }

}

TEST_CASE("Packet::packetsBetween()", "[Packet]") {

    int packetSize = 0;
    int size = PRELOAD_BYTES + POSTLOAD_BYTES + packetSize;

    SECTION("packetsBetween() is zero between consecutive packets") {

        unsigned char data[size];

        data[size - 2] = 0x01;
        data[size - 1] = 0x02;

        unsigned char data2[size];

        data2[size - 2] = 0x01;
        data2[size - 1] = 0x03;

        Packet packet(data, size);
        Packet packet2(data2, size);

        REQUIRE(Packet::packetsBetween(packet, packet2) == 0);

    }

    SECTION("packetsBetween() is maximal between packets with the same packet number") {

        unsigned char data[size];

        data[size - 2] = 0x01;
        data[size - 1] = 0x02;

        unsigned char data2[size];

        data2[size - 2] = 0x01;
        data2[size - 1] = 0x02;

        Packet packet(data, size);
        Packet packet2(data2, size);

        REQUIRE(Packet::packetsBetween(packet, packet2) == 0xFFFF);

    }

    SECTION("packetsBetween() is symmetric") {

        unsigned char data[size];

        data[size - 2] = 0x92;
        data[size - 1] = 0x3A;

        unsigned char data2[size];

        data2[size - 2] = 0x46;
        data2[size - 1] = 0xF3;

        Packet packet(data, size);
        Packet packet2(data2, size);
        
        REQUIRE(
            Packet::packetsBetween(packet, packet2) == 
            Packet::packetsBetween(packet2, packet)
        );

    }

    SECTION("packetsBetween() is correct across the overflow boundary") {

        unsigned char data[size];

        data[size - 2] = 0xFF;
        data[size - 1] = 0xFF;

        unsigned char data2[size];

        data2[size - 2] = 0x00;
        data2[size - 1] = 0x00;

        Packet packet(data, size);
        Packet packet2(data2, size);

        REQUIRE(Packet::packetsBetween(packet, packet2) == 0);

    }

    SECTION("packetsBetween() is correct for extrema not crossing overflow boundary") {

        unsigned char data[size];

        data[size - 2] = 0x00;
        data[size - 1] = 0x00;

        unsigned char data2[size];

        data2[size - 2] = 0xFF;
        data2[size - 1] = 0xFF;

        Packet packet(data, size);
        Packet packet2(data2, size);

        REQUIRE(Packet::packetsBetween(packet, packet2) == 0xFFFE);

    }

    SECTION("packetsBetween() is correct for smaller first packet") {

        unsigned char data[size];

        data[size - 2] = 0x12;
        data[size - 1] = 0x53;

        unsigned char data2[size];

        data2[size - 2] = 0x55;
        data2[size - 1] = 0x64;

        Packet packet(data, size);
        Packet packet2(data2, size);

        REQUIRE(Packet::packetsBetween(packet, packet2) == 0x4310);

    }

    SECTION("packetsBetween() is correct for smaller second packet") {

        unsigned char data[size];

        data[size - 2] = 0x55;
        data[size - 1] = 0x64;

        unsigned char data2[size];

        data2[size - 2] = 0x12;
        data2[size - 1] = 0x53;

        Packet packet(data, size);
        Packet packet2(data2, size);

        REQUIRE(Packet::packetsBetween(packet, packet2) == 0xBCEE);

    }

}

TEST_CASE("Packet::const_iterator", "[Packet]") {

    SECTION("Packet::cbegin() refers to the correct byte") {

        size_t packetSize = 10;
        size_t size = PRELOAD_BYTES + POSTLOAD_BYTES + packetSize;

        unsigned char data[size];
        for(int i = 0; i < PRELOAD_BYTES; ++i) {
            data[i] = 0;
        }
        for(int i = PRELOAD_BYTES; i < PRELOAD_BYTES + packetSize; ++i) {
            data[i] = i - PRELOAD_BYTES + 1;
        }
        for(int i = size; i --> size - POSTLOAD_BYTES; i) {
            data[i] = 0;
        }
        Packet packet(data, size);

        REQUIRE(*packet.cbegin() == data[PRELOAD_BYTES]);

    }

    SECTION("Packet::cend() refers to the correct byte") {

        size_t packetSize = 10;
        size_t size = PRELOAD_BYTES + POSTLOAD_BYTES + packetSize;

        unsigned char data[size];
        for(int i = 0; i < PRELOAD_BYTES; ++i) {
            data[i] = 0;
        }
        for(int i = PRELOAD_BYTES; i < PRELOAD_BYTES + packetSize; ++i) {
            data[i] = i - PRELOAD_BYTES + 1;
        }
        for(int i = size; i --> size - POSTLOAD_BYTES; i) {
            data[i] = 0;
        }
        Packet packet(data, size);

        REQUIRE(*(packet.cend() - 1) == data[size - POSTLOAD_BYTES - 1]);

    }

    SECTION("Iterating over Packet gives correct values") {

        size_t packetSize = 10;
        size_t size = PRELOAD_BYTES + POSTLOAD_BYTES + packetSize;

        unsigned char data[size];
        for(int i = 0; i < PRELOAD_BYTES; ++i) {
            data[i] = 0;
        }
        for(int i = PRELOAD_BYTES; i < PRELOAD_BYTES + packetSize; ++i) {
            data[i] = i - PRELOAD_BYTES + 1;
        }
        for(int i = size; i --> size - POSTLOAD_BYTES; i) {
            data[i] = 0;
        }
        Packet packet(data, size);

        for(auto it = packet.cbegin(); it != packet.cend(); ++it) {
            REQUIRE(*it == data[(it - packet.cbegin()) + PRELOAD_BYTES]);
        }

    }

}

TEST_CASE("Packet::operator[]", "[Packet]") {
    
    SECTION("Packet::operator[] throws if the index is out of range") {

        size_t packetSize = 10;
        size_t size = PRELOAD_BYTES + POSTLOAD_BYTES + packetSize;

        unsigned char data[size];
        for(int i = 0; i < PRELOAD_BYTES; ++i) {
            data[i] = 0;
        }
        for(int i = PRELOAD_BYTES; i < PRELOAD_BYTES + packetSize; ++i) {
            data[i] = i - PRELOAD_BYTES + 1;
        }
        for(int i = size; i --> size - POSTLOAD_BYTES; i) {
            data[i] = 0;
        }
        Packet packet(data, size);

        REQUIRE_THROWS_AS(packet[packetSize], std::out_of_range);

    }

    SECTION("Packet::operator[] returns the correct byte") {

        size_t packetSize = 10;
        size_t size = PRELOAD_BYTES + POSTLOAD_BYTES + packetSize;

        unsigned char data[size];
        for(int i = 0; i < PRELOAD_BYTES; ++i) {
            data[i] = 0;
        }
        for(int i = PRELOAD_BYTES; i < PRELOAD_BYTES + packetSize; ++i) {
            data[i] = i - PRELOAD_BYTES + 1;
        }
        for(int i = size; i --> size - POSTLOAD_BYTES; i) {
            data[i] = 0;
        }
        Packet packet(data, size);

        for(int i = 0; i < packetSize; ++i) {
            REQUIRE(packet[i] == data[i + PRELOAD_BYTES]);
        }

    }

}