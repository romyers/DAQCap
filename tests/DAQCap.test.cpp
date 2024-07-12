#include <catch2/catch_test_macros.hpp>

#include <DAQCap.h>

TEST_CASE("DAQCap::SessionHandler::getDeviceList calls internal", "[DAQCap]") {

    std::vector<DAQCap::Device> devices 
        = DAQCap::SessionHandler::getDeviceList();

    REQUIRE(devices.size() == 2);

    REQUIRE(devices[0].name == "MockDeviceName");
    REQUIRE(devices[0].description == "MockDeviceDescription");

    REQUIRE(devices[1].name == "MockDevice2Name");
    REQUIRE(devices[1].description == "MockDevice2Description");

}

TEST_CASE("DAQCap::SessionHandler::timeout_exception", "[DAQCap]") {

    DAQCap::timeout_exception e("Test exception");

    REQUIRE(e.what() == std::string("Test exception"));

}

TEST_CASE("DAQCap::SessionHandler", "[DAQCap]") {
    
    DAQCap::Device device;
    DAQCap::SessionHandler session(device);

    SECTION("fetchPacket() times out") {

        REQUIRE_THROWS_AS(session.fetchPackets(5, 1), DAQCap::timeout_exception);

    }

    SECTION("Interrupt calls internal") {

        session.interrupt();

        DAQCap::DataBlob data = session.fetchPackets(1000, 30);

        // The mock implementation of NetworkInterface just returns an empty
        // vector of packets if it's interrupted.
        REQUIRE(data.packetCount == 0);

    }

    SECTION("fetchPacket() returns correct packet count") {

        DAQCap::DataBlob data = session.fetchPackets(1000, 30);

        REQUIRE(data.packetCount == 30);

        data = session.fetchPackets(1000, 500);

        REQUIRE(data.packetCount == 100);

    }

    SECTION("fetchPackets() returns correct packets") {

        DAQCap::DataBlob data = session.fetchPackets(1000, 2);

        for(int i = 0; i < 256; ++i) {

            REQUIRE(data.data[i] == (i + 14) % 256);

        }

        for(int i = 256; i < 512; ++i) {

            REQUIRE(data.data[i] == (i + 14 + 1) % 256);

        }

    }

    SECTION("checkPacketNumbers()") {

        REQUIRE(false);

    }

}