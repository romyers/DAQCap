#include <catch2/catch_test_macros.hpp>

#include <DAQCap.h>
#include <NetworkInterface.h>

#include <thread>

using namespace DAQCap;

// TODO: We can singleton this and use it to get info out of MockManagers
//       created by the create() function.

// TODO: All that's left really is to write these tests and write some
//       integration tests for the standalone program.

class MockManager: public NetworkManager {

public:

    MockManager() = default;
    ~MockManager();

    void startSession(const Device &device) override;

    bool hasOpenSession() override;

    void endSession() override;

    MockManager(const MockManager &other)            = delete;
    MockManager &operator=(const MockManager &other) = delete;

    void interrupt();
    std::vector<Packet> fetchPackets(int packetsToRead);

    std::vector<Device> getAllDevices() override;

private:

    bool interrupted = false;
    bool sessionOpen = false;

};

NetworkManager *NetworkManager::create() {

    return new MockManager();

}

std::vector<Device> MockManager::getAllDevices() {

    std::vector<Device> devices;

    devices.push_back(
        Device("MockDeviceName", "MockDeviceDescription")
    );
    devices.push_back(
        Device("MockDevice2Name", "MockDevice2Description")
    );

    return devices;

}

MockManager::~MockManager() {

}

void MockManager::startSession(const Device &device) {

    if(hasOpenSession()) {

        throw std::logic_error(
            "MockManager::startSession() cannot be called while another "
            "session is open."
        );

    }

    sessionOpen = true;

}

bool MockManager::hasOpenSession() {

    return sessionOpen;

}

void MockManager::endSession() {

    sessionOpen = false;

}

void MockManager::interrupt() {

    interrupted = true;

}

std::vector<Packet> MockManager::fetchPackets(int packetsToRead) {

    std::vector<Packet> packets;

    if(interrupted) {

        interrupted = false;
        return packets;

    }

    for(int i = 0; i < std::min(packetsToRead, 100); ++i) {

        int size = 14 + 256 + 4;

        uint8_t *data = new uint8_t[size];

        for(int j = 0; j < 14; ++j) {

            data[j] = 0;

        }

        for(int j = 14; j < size - 4; ++j) {

            data[j] = (j + i) % 256;

        }

        data[size - 4] = 0;
        data[size - 3] = 0;
        data[size - 2] = i / 256;
        data[size - 1] = i % 256;

        packets.emplace_back(data, size);

    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    return packets;

}

TEST_CASE("FORCEFAIL") {

    REQUIRE(false);

}

// TODO: Benchmarking using Catch2's benchmark macro
// TODO: Mocking framework

TEST_CASE("DAQCap::SessionHandler::getAllNetworkDevices calls internal", "[DAQCap]") {

    DAQCap::SessionHandler handler;

    std::vector<DAQCap::Device> devices 
        = handler.getAllNetworkDevices();

    REQUIRE(devices.size() == 2);

    REQUIRE(devices[0].getName() == "MockDeviceName");
    REQUIRE(devices[0].getDescription() == "MockDeviceDescription");

    REQUIRE(devices[1].getName() == "MockDevice2Name");
    REQUIRE(devices[1].getDescription() == "MockDevice2Description");

}

TEST_CASE("DAQCap::SessionHandler::timeout_exception", "[DAQCap]") {

    DAQCap::timeout_exception e("Test exception");

    REQUIRE(e.what() == std::string("Test exception"));

}

TEST_CASE("DAQCap::SessionHandler", "[DAQCap]") {
    
    DAQCap::SessionHandler session;
    session.startSession(session.getAllNetworkDevices()[0]);

    SECTION("fetchPacket() times out") {

        REQUIRE_THROWS_AS(
            session.fetchData(std::chrono::milliseconds(5), 1), 
            DAQCap::timeout_exception
        );

    }

    SECTION("Interrupt calls internal") {

        session.interrupt();

        DAQCap::DataBlob data = session.fetchData(std::chrono::seconds(1), 30);

        // The mock implementation of NetworkInterface just returns an empty
        // vector of packets if it's interrupted.
        REQUIRE(data.packetCount() == 0);

    }

    SECTION("fetchPacket() returns correct packet count") {

        DAQCap::DataBlob data = session.fetchData(std::chrono::seconds(1), 30);

        REQUIRE(data.packetCount() == 30);

        data = session.fetchData(std::chrono::seconds(1), 500);

        REQUIRE(data.packetCount() == 100);

    }

    SECTION("fetchData() returns correct packets") {

        DAQCap::DataBlob data = session.fetchData(std::chrono::seconds(1), 2);

        for(int i = 0; i < 256; ++i) {

            REQUIRE(data.data()[i] == (i + 14) % 256);

        }

        for(int i = 256; i < 512; ++i) {

            REQUIRE(data.data()[i] == (i + 14 + 1) % 256);

        }

    }

    SECTION("checkPacketNumbers()") {

        REQUIRE(false);

    }

}