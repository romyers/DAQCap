#include <catch2/catch_test_macros.hpp>

#include <DAQCap.h>
#include <NetworkInterface.h>

#include <thread>
#include <numeric>
#include <sstream>

using std::vector;
using std::string;

using std::shared_ptr;

using namespace DAQCap;

const size_t WORD_SIZE = 5;

const size_t PRELOAD = 14;
const size_t POSTLOAD = 4;

class MockDevice : public Device {

public:

    MockDevice(string name, string description) 
        : name(name), description(description) {}

    virtual string getName() const { return name; }

    virtual string getDescription() const { return description; }

    string name;
    string description;

    vector<Packet> packets;

    bool throwEverything = false;

};

vector<shared_ptr<MockDevice>> g_devices;

class MockManager: public NetworkManager {

public:

    MockManager();
    ~MockManager();

    void startSession(const shared_ptr<Device> device) override;

    void endSession() override;

    void interrupt() override;
    vector<Packet> fetchPackets(int packetsToRead) override;

    vector<shared_ptr<Device>> getAllDevices() override;

    bool isInterrupted() { return interrupted; }

private:

    bool interrupted = false;
    bool sessionOpen = false;

    shared_ptr<MockDevice> mock = nullptr;

};

MockManager::MockManager() {

    g_devices.clear();

    g_devices.emplace_back(
        new MockDevice("MockDeviceName", "MockDeviceDescription")
    );

    g_devices.emplace_back(
        new MockDevice("MockDevice2Name", "MockDevice2Description")
    );

}

std::unique_ptr<NetworkManager> NetworkManager::create() {

    return std::unique_ptr<NetworkManager>(new MockManager);

}

vector<shared_ptr<Device>> MockManager::getAllDevices() {

    vector<shared_ptr<Device>> devices;

    for(shared_ptr<MockDevice> d : g_devices) {

        if(d->throwEverything) {

            throw std::runtime_error("MockManager::getAllDevices()");

        }

        devices.push_back(d);

    }

    return devices;

}

MockManager::~MockManager() {

}

void MockManager::startSession(const shared_ptr<Device> device) {

    if(sessionOpen) {

        throw std::logic_error(
            "MockManager::startSession() cannot be called while another "
            "session is open."
        );

    }

    try {

        // This is silly testing stuff, but we can allow some silly testing
        // stuff
        const shared_ptr<MockDevice> mockDevice 
            = std::dynamic_pointer_cast<MockDevice>(device);
        mock = mockDevice;

    } catch(...) {

        return;

    }

    sessionOpen = true;
    interrupted = false;

    if(mock->throwEverything) {

        throw std::runtime_error("MockManager::startSession()");

    }

}

void MockManager::endSession() {

    sessionOpen = false;
    mock = nullptr;

}

void MockManager::interrupt() {

    interrupted = true;

}

vector<Packet> MockManager::fetchPackets(int packetsToRead) {

    if(!sessionOpen) {

        throw std::runtime_error("No open session");

    }

    if(mock->throwEverything) {

        throw std::runtime_error("MockManager::fetchPackets()");

    }

    if(interrupted) {

        interrupted = false;
        return vector<Packet>();

    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    vector<Packet> temp = mock->packets;

    mock->packets.clear();

    return temp;

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("DAQCap::packData()") {

    SECTION("packData() returns empty for empty input") {

        vector<uint8_t> empty;

        vector<Word> packedData = packData(empty);

        REQUIRE(packedData.empty());

    }

    SECTION("packData() returns empty for input smaller than wordsize") {

        vector<uint8_t> small(WORD_SIZE - 1);

        vector<Word> packedData = packData(small);

        REQUIRE(packedData.empty());

    }

    SECTION("packData() is correct for input of exactly wordsize") {

        vector<uint8_t> data(WORD_SIZE);

        std::iota(data.begin(), data.end(), 0);

        vector<Word> packedData = packData(data);

        REQUIRE(packedData.size() == 1);
        REQUIRE(packedData[0] == 0x0001020304);

    }

    SECTION("packData() is correct for one word plus a partial word") {

        vector<uint8_t> data(WORD_SIZE + 1);

        std::iota(data.begin(), data.end(), 0);

        vector<Word> packedData = packData(data);

        REQUIRE(packedData.size() == 1);
        REQUIRE(packedData[0] == 0x0001020304);

    }

    SECTION("packData() is correct for multiple words") {

        vector<uint8_t> data(WORD_SIZE * 3);

        std::iota(data.begin(), data.end(), 0);

        vector<Word> packedData = packData(data);

        REQUIRE(packedData.size() == 3);
        REQUIRE(packedData[0] == 0x0001020304);
        REQUIRE(packedData[1] == 0x0506070809);
        REQUIRE(packedData[2] == 0x0a0b0c0d0e);

    }

    SECTION("packData() is correct for multiple words plus a partial") {

        vector<uint8_t> data(WORD_SIZE * 4 - 1);

        std::iota(data.begin(), data.end(), 0);

        vector<Word> packedData = packData(data);

        REQUIRE(packedData.size() == 3);
        REQUIRE(packedData[0] == 0x0001020304);
        REQUIRE(packedData[1] == 0x0506070809);
        REQUIRE(packedData[2] == 0x0a0b0c0d0e);

    }

}

TEST_CASE("SessionHandler::getAllNetworkDevices()") {
           
    SessionHandler handler;

    SECTION("getAllNetworkDevices() is correct on first call") {

        vector<shared_ptr<Device>> devices = handler.getAllNetworkDevices();

        REQUIRE(devices.size() == 2);
        REQUIRE(devices[0]->getName() == "MockDeviceName");
        REQUIRE(devices[1]->getName() == "MockDevice2Name");

    }

    SECTION("getAllNetworkDevices() is kept up-to-date") {

        vector<shared_ptr<Device>> devices = handler.getAllNetworkDevices();

        g_devices.emplace_back(
            new MockDevice("MockDevice3Name", "MockDevice3Description")
        );

        devices = handler.getAllNetworkDevices();

        REQUIRE(devices.size() == 3);
        REQUIRE(devices[0]->getName() == "MockDeviceName");
        REQUIRE(devices[1]->getName() == "MockDevice2Name");
        REQUIRE(devices[2]->getName() == "MockDevice3Name");

        g_devices.pop_back();

    }

    SECTION("getAllNetworkDevices() throws on error") {

        g_devices[0]->throwEverything = true;

        REQUIRE_THROWS(handler.getAllNetworkDevices());

        g_devices[0]->throwEverything = false;

    }

}

TEST_CASE("SessionHandler::getNetworkDevice()") {

    SessionHandler handler;

    SECTION("getNetworkDevice() returns nullptr for empty name") {

        shared_ptr<Device> device = handler.getNetworkDevice("");

        REQUIRE(device == nullptr);

    }

    SECTION("getNetworkDevice() returns nullptr for nonexistant device") {

        shared_ptr<Device> device = handler.getNetworkDevice("Nonexistant");

        REQUIRE(device == nullptr);

    }

    SECTION("getNetworkDevice() returns correct device") {

        shared_ptr<Device> device = handler.getNetworkDevice("MockDeviceName");

        REQUIRE(device != nullptr);
        REQUIRE(device->getName() == "MockDeviceName");

    }

    SECTION("getNetworkDevice() is kept up-to-date") {

        shared_ptr<Device> device = handler.getNetworkDevice("MockDeviceName");

        g_devices.emplace_back(
            new MockDevice("MockDevice3Name", "MockDevice3Description")
        );

        device = handler.getNetworkDevice("MockDevice3Name");

        REQUIRE(device != nullptr);
        REQUIRE(device->getName() == "MockDevice3Name");

        g_devices.pop_back();

    }

    SECTION("getNetworkDevice() throws on error") {

        g_devices[0]->throwEverything = true;

        REQUIRE_THROWS(handler.getNetworkDevice("MockDeviceName"));

        g_devices[0]->throwEverything = false;

    }

}

TEST_CASE("SessionHandler::startSession()", "[SessionHandler]") {

    SessionHandler handler;

    SECTION("startSession() quietly handles if session is already open") {

        handler.startSession(g_devices[0]);

        REQUIRE_NOTHROW(handler.startSession(g_devices[0]));

    }

    SECTION("startSession() works for not the first device") {

        REQUIRE_NOTHROW(handler.startSession(g_devices[1]));

    }

    SECTION("startSession() throws if device is null") {

        REQUIRE_THROWS(handler.startSession(nullptr));

    }

    SECTION("startSession() throws if device does not exist") {

        shared_ptr<Device> device(
            new MockDevice("Nonexistant", "Nonexistant")
        );

        REQUIRE_THROWS(handler.startSession(device));

    }

    SECTION("startSession() throws on error") {

        g_devices[0]->throwEverything = true;

        REQUIRE_THROWS(handler.startSession(g_devices[0]));

        g_devices[0]->throwEverything = false;

    }

    SECTION("startSession() starts the session") {

        handler.startSession(g_devices[0]);

        // This would throw if the session hadn't started
        REQUIRE_NOTHROW(handler.fetchData());

    }

}

TEST_CASE("SessionHandler::endSession()") {

    SessionHandler handler;

    SECTION("endSession() quietly handles if no session is open") {

        REQUIRE_NOTHROW(handler.endSession());

    }

    SECTION("endSession() ends the session") {

        handler.startSession(g_devices[0]);

        handler.endSession();

        // This would not throw if the session had not ended
        REQUIRE_THROWS(handler.fetchData());

    }

}

TEST_CASE("SessionHandler::fetchData()") {

    SessionHandler handler;

    g_devices[0]->packets.clear();

    SECTION("fetchData() throws if no session is open") {

        REQUIRE_THROWS(handler.fetchData());

    }

    handler.startSession(g_devices[0]);

    SECTION("fetchData() throws on error") {

        g_devices[0]->throwEverything = true;

        REQUIRE_THROWS(handler.fetchData());

        g_devices[0]->throwEverything = false;

    }

    SECTION("fetchData() times out if it takes too long") {

        REQUIRE_THROWS_AS(
            handler.fetchData(std::chrono::milliseconds(1)), 
            timeout_exception
        );

    }

    SECTION("fetchData() does not time out if it's fast enough") {

        REQUIRE_NOTHROW(handler.fetchData(std::chrono::milliseconds(10)));

    }

    SECTION("fetchData() stream insertion") {

        size_t packetSize = WORD_SIZE;
        size_t size = PRELOAD + POSTLOAD + packetSize;

        vector<uint8_t> data (size, 0);

        std::iota(data.begin() + PRELOAD, data.end() - POSTLOAD, 0);

        Packet packet(data.data(), data.size());

        g_devices[0]->packets.push_back(packet);

        std::stringstream ss;

        ss << handler.fetchData();

        REQUIRE(ss.str().size() == packetSize);

        for(int i = 0; i < ss.str().size(); ++i) {

            REQUIRE(ss.str()[i] == i);

        }

    }

}

TEST_CASE("SessionHandler::interrupt()") {

    SessionHandler handler;

    // Let's prepare a device with data, so we can make sure an interrupted
    // session handler doesn't return the data.
    shared_ptr<MockDevice> device(
        new MockDevice("MockDeviceName", "MockDeviceDescription")
    );

    vector<uint8_t> data(PRELOAD + POSTLOAD + WORD_SIZE * 3, 0);

    for(int i = PRELOAD; i < data.size() - POSTLOAD; ++i) {

        data[i] = i;

    }

    Packet packet(data.data(), data.size());

    device->packets.push_back(packet);

    handler.startSession(shared_ptr<Device>(device));

    // Now we'll interrupt the handler and verify that fetching returns empty.
    // The mock implementation lets us interrupt ahead of time.
    handler.interrupt();
    DataBlob blob = handler.fetchData();

    REQUIRE(blob.packetCount() == 0);
    REQUIRE(blob.data().empty());

}