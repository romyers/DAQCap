#include <catch2/catch_test_macros.hpp>

#include <Packet.h>

TEST_CASE("Packet -- constructor populates data and size", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    REQUIRE(packet.size == 4);
    REQUIRE(packet.data != nullptr);

    for (size_t i = 0; i < packet.size; i++) {

        REQUIRE(packet.data[i] == data[i]);
        
    }

}

TEST_CASE("Packet -- constructor copies data", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    data[0] = 0x00;
    data[3] = 0x05;

    REQUIRE(packet.size == 4);
    REQUIRE(packet.data != nullptr);
    
    REQUIRE(packet.data[0] == 0x01);
    REQUIRE(packet.data[3] == 0x04);

}

TEST_CASE("Packet -- copy constructor copies data", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    DAQCap::Packet packet2(packet);

    packet.data[0] = 0x00;

    REQUIRE(packet2.data != nullptr);

    for (size_t i = 0; i < 4; i++) {

        REQUIRE(packet2.data[i] == data[i]);
        
    }

}

TEST_CASE("Packet -- copy constructor copies size", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    DAQCap::Packet packet2(packet);

    REQUIRE(packet2.size == 4);

}

TEST_CASE("Packet -- copy assignment copies data", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    unsigned char data2[] = {0x05, 0x06, 0x07, 0x08};
    DAQCap::Packet packet2(data2, 4);

    packet2 = packet;

    packet.data[0] = 0x00;

    REQUIRE(packet2.data != nullptr);

    for (size_t i = 0; i < 4; i++) {

        REQUIRE(packet2.data[i] == data[i]);
        
    }

}

TEST_CASE("Packet -- copy assignment copies size", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    unsigned char data2[] = {0x05, 0x06, 0x07};
    DAQCap::Packet packet2(data2, 3);

    packet2 = packet;

    REQUIRE(packet2.size == 4);

}

TEST_CASE("Packet -- copy assignment returns new packet", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    unsigned char data2[] = {0x05, 0x06, 0x07};
    DAQCap::Packet packet2(data2, 3);

    REQUIRE(&packet2 == &(packet2 = packet));

}

TEST_CASE("Packet -- move constructor deletes old packet", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    DAQCap::Packet packet2(std::move(packet));

    REQUIRE(packet.data == nullptr);
    REQUIRE(packet.size == 0);

}

TEST_CASE("Packet -- move constructor moves data", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    DAQCap::Packet packet2(std::move(packet));

    REQUIRE(packet2.data != nullptr);
    REQUIRE(packet2.size == 4);

    for (size_t i = 0; i < 4; i++) {

        REQUIRE(packet2.data[i] == data[i]);
        
    }

}

TEST_CASE("Packet -- move assignment deletes old packet", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    unsigned char data2[] = {0x05, 0x06, 0x07, 0x08};
    DAQCap::Packet packet2(data2, 4);

    packet2 = std::move(packet);

    REQUIRE(packet.data == nullptr);
    REQUIRE(packet.size == 0);

}

TEST_CASE("Packet -- move assignment moves data", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    unsigned char data2[] = {0x05, 0x06, 0x07, 0x08};
    DAQCap::Packet packet2(data2, 4);

    packet2 = std::move(packet);

    REQUIRE(packet2.data != nullptr);
    REQUIRE(packet2.size == 4);

    for (size_t i = 0; i < 4; i++) {

        REQUIRE(packet2.data[i] == data[i]);
        
    }

}

TEST_CASE("Packet -- move assignment returns new packet", "[Packet]") {

    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    DAQCap::Packet packet(data, 4);

    unsigned char data2[] = {0x05, 0x06, 0x07, 0x08};
    DAQCap::Packet packet2(data2, 4);

    REQUIRE(&packet2 == &(packet2 = std::move(packet)));

}