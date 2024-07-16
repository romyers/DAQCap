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

    bool hasOpenSession() override;

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

    if(hasOpenSession()) {

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

bool MockManager::hasOpenSession() {

    return sessionOpen;

}

void MockManager::endSession() {

    sessionOpen = false;
    mock = nullptr;

}

void MockManager::interrupt() {

    interrupted = true;

}

vector<Packet> MockManager::fetchPackets(int packetsToRead) {

    if(mock->throwEverything) {

        throw std::runtime_error("MockManager::fetchPackets()");

    }

    if(interrupted) {

        interrupted = false;
        return vector<Packet>();

    }

    if(!hasOpenSession()) {

        throw std::runtime_error("No open session");

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

    SECTION("getAllNetworkDevices() is correct after caching") {

        vector<shared_ptr<Device>> devices = handler.getAllNetworkDevices();

        g_devices.emplace_back(
            new MockDevice("MockDevice3Name", "MockDevice3Description")
        );

        devices = handler.getAllNetworkDevices();

        REQUIRE(devices.size() == 2);
        REQUIRE(devices[0]->getName() == "MockDeviceName");
        REQUIRE(devices[1]->getName() == "MockDevice2Name");

        g_devices.pop_back();

    }

    SECTION("getAllNetworkDevices() is correct with cache reload") {

        vector<shared_ptr<Device>> devices = handler.getAllNetworkDevices();

        g_devices.emplace_back(
            new MockDevice("MockDevice3Name", "MockDevice3Description")
        );

        handler.clearDeviceCache();
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

    SECTION("getNetworkDevice() does not relaod cache") {

        shared_ptr<Device> device = handler.getNetworkDevice("MockDeviceName");

        g_devices.emplace_back(
            new MockDevice("MockDevice3Name", "MockDevice3Description")
        );

        device = handler.getNetworkDevice("MockDevice3Name");

        REQUIRE(device == nullptr);

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

    SECTION("fetchData() returns empty if no packets are available") {

        DataBlob blob = handler.fetchData();

        REQUIRE(blob.packetCount() == 0);
        REQUIRE(blob.data().empty());

    }

    SECTION("fetchData() correctly handles empty packets") {

        g_devices[0]->packets.emplace_back(
            std::vector<uint8_t>(PRELOAD + POSTLOAD, 0).data(), 
            PRELOAD + POSTLOAD
        );

        DataBlob blob = handler.fetchData();

        REQUIRE(blob.packetCount() == 1);

        REQUIRE(blob.data().empty());

        g_devices[0]->packets.clear();

    }

    SECTION("fetchData() returns correct data") {

        // Here we just generate a bunch of unique data and check every byte
        // in the blob

        size_t size = WORD_SIZE * 3;

        vector<uint8_t> data(PRELOAD + POSTLOAD + size, 0);
        vector<uint8_t> data2(PRELOAD + POSTLOAD + size, 0);

        std::iota(data.begin() + PRELOAD, data.end() - POSTLOAD, 0);
        std::iota(data2.begin() + PRELOAD, data2.end() - POSTLOAD, size);

        Packet packet(data.data(), data.size());
        Packet packet2(data2.data(), data2.size());

        g_devices[0]->packets.push_back(packet);
        g_devices[0]->packets.push_back(packet2);

        DataBlob blob = handler.fetchData();

        REQUIRE(blob.packetCount() == 2);
        REQUIRE(blob.data().size() == size * blob.packetCount());

        for(int i = 0; i < blob.data().size(); ++i) {

            REQUIRE(blob.data()[i] == i);

        }

        g_devices[0]->packets.clear();

    }

    SECTION("fetchData() throws on error") {

        g_devices[0]->throwEverything = true;

        REQUIRE_THROWS(handler.fetchData());

        g_devices[0]->throwEverything = false;

    }

    SECTION("fetchData() with partial words") {

        int FIRST_VAL  = 1;
        int SECOND_VAL = 2;
        int THIRD_VAL  = 3;

        int FIRST_SIZE  = 2 * WORD_SIZE + 1;
        int SECOND_SIZE = 3 * WORD_SIZE;
        int THIRD_SIZE  = WORD_SIZE - 1;

        // First packet will have two words plus an extra byte
        vector<uint8_t> data(
            PRELOAD + POSTLOAD + FIRST_SIZE, 
            FIRST_VAL
        );

        // Second packet will have three words
        vector<uint8_t> data2(
            PRELOAD + POSTLOAD + SECOND_SIZE, 
            SECOND_VAL
        );

        Packet packet (data .data(), data .size());
        Packet packet2(data2.data(), data2.size());

        g_devices[0]->packets.push_back(packet);
        g_devices[0]->packets.push_back(packet2);

        DataBlob blob = handler.fetchData();

        REQUIRE(blob.packetCount() == 2);
        
        // The partial word in the first packet should be completed by the
        // second packet, leaving a partial at the end, which should be
        // discarded.
        REQUIRE(
            blob.data().size() 
            == 
            FIRST_SIZE + SECOND_SIZE - (FIRST_SIZE + SECOND_SIZE) % WORD_SIZE
        );

        // Starts in the right place
        REQUIRE(blob.data()[0] == FIRST_VAL);

        // Left side of packet boundary is correct
        REQUIRE(blob.data()[FIRST_SIZE - 1] == FIRST_VAL);

        // Right side of packet boundary is correct
        REQUIRE(blob.data()[FIRST_SIZE] == SECOND_VAL);

        // Now let's make sure partial words across fetch calls are handled
        // properly.
        // We'll make a packet that finishes the word from the last packet
        vector<uint8_t> data3(
            PRELOAD + POSTLOAD + THIRD_SIZE, 
            THIRD_VAL
        );

        Packet packet3(data3.data(), data3.size());

        g_devices[0]->packets.push_back(packet3);

        blob = handler.fetchData();

        // Check the packet count
        REQUIRE(blob.packetCount() == 1);

        // The first byte should be from the last packet
        REQUIRE(blob.data()[0] == SECOND_VAL);

        // And the byte after should be from the new packet
        REQUIRE(blob.data()[1] == THIRD_VAL);

        g_devices[0]->packets.clear();

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

    SECTION("fetchData() reports missing packets correctly") {

        size_t packetSize = WORD_SIZE;
        size_t size = PRELOAD + POSTLOAD + packetSize;

        g_devices[0]->packets.clear();

        vector<uint8_t> data (size, 0);
        vector<uint8_t> data2(size, 0);
        vector<uint8_t> data3(size, 0);

        SECTION("fetchData() missing packets do not affect received packets") {

            data [size - 1] = 1;
            data2[size - 1] = 2;
            data3[size - 1] = 5;

            g_devices[0]->packets.emplace_back(data .data(), data .size());
            g_devices[0]->packets.emplace_back(data2.data(), data2.size());
            g_devices[0]->packets.emplace_back(data3.data(), data3.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.packetCount() == 3);
            REQUIRE(blob.data().size() == packetSize * 3);

        }

        SECTION("fetchData() reports missing packets within a fetch call") {

            data [size - 1] = 1;
            data2[size - 1] = 2;
            data3[size - 1] = 5;

            g_devices[0]->packets.emplace_back(data .data(), data .size());
            g_devices[0]->packets.emplace_back(data2.data(), data2.size());
            g_devices[0]->packets.emplace_back(data3.data(), data3.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.warnings().size() == 1);

            REQUIRE(
                blob.warnings().front() == "2 packets lost! Packet = 5, Last = 2"
            );

        }

        SECTION("fetchData() reports missing packets across fetch calls") {

            data [size - 1] = 1;
            data2[size - 1] = 2;
            data3[size - 1] = 5;

            g_devices[0]->packets.emplace_back(data .data(), data .size());
            g_devices[0]->packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.warnings().empty());

            g_devices[0]->packets.emplace_back(data3.data(), data3.size());

            blob = handler.fetchData();

            REQUIRE(blob.warnings().size() == 1);

            REQUIRE(
                blob.warnings().front() == "2 packets lost! Packet = 5, Last = 2"
            );

        }

        SECTION("fetchData() reports multiple missing packets") {

            data [size - 1] = 1;
            data2[size - 1] = 3;
            data3[size - 1] = 5;

            g_devices[0]->packets.emplace_back(data.data(), data.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.warnings().empty());

            g_devices[0]->packets.emplace_back(data2.data(), data2.size());
            g_devices[0]->packets.emplace_back(data3.data(), data3.size());

            blob = handler.fetchData();

            REQUIRE(blob.warnings().size() == 2);

            REQUIRE(
                blob.warnings().front() == "1 packets lost! Packet = 3, Last = 1"
            );

            REQUIRE(
                blob.warnings().back() == "1 packets lost! Packet = 5, Last = 3"
            );

        }

        SECTION("fetchData() reports missing packets for identical packet numbers") {

            data [size - 1] = 1;
            data2[size - 1] = 1;

            g_devices[0]->packets.emplace_back(data.data(), data.size());
            g_devices[0]->packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.warnings().size() == 1);

            REQUIRE(
                blob.warnings().front() == "65535 packets lost! Packet = 1, Last = 1"
            );

        }

        SECTION("fetchData() reports missing packets across overflow boundary") {

            data [size - 1] = 3;
            data2[size - 1] = 1;

            g_devices[0]->packets.emplace_back(data .data(), data .size());
            g_devices[0]->packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.warnings().size() == 1);

            REQUIRE(
                blob.warnings().front() == "65533 packets lost! Packet = 1, Last = 3"
            );

        }

        SECTION("fetchData() does not report consecutive packets within a fetch call") {

            data [size - 1] = 1;
            data2[size - 1] = 2;

            g_devices[0]->packets.emplace_back(data .data(), data .size());
            g_devices[0]->packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.warnings().empty());

        }

        SECTION("fetchData() does not report consecutive packets across fetch calls") {

            data [size - 1] = 1;
            data2[size - 1] = 2;

            g_devices[0]->packets.emplace_back(data.data(), data.size());

            DataBlob blob = handler.fetchData();

            g_devices[0]->packets.emplace_back(data2.data(), data2.size());

            blob = handler.fetchData();

            REQUIRE(blob.warnings().empty());


        }

        SECTION("fetchData() does not report consecutive packets across overflow boundary") {

            data [size - 1] = 0xFF;
            data [size - 2] = 0xFF;

            data2[size - 1] = 0;

            g_devices[0]->packets.emplace_back(data .data(), data .size());
            g_devices[0]->packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.warnings().empty());

        }

    }

    SECTION("fetchData() idle word removal") {

        g_devices[0]->packets.clear();

        SECTION("fetchData() removes idle words") {

            size_t packetSize = WORD_SIZE;
            size_t size = PRELOAD + POSTLOAD + packetSize;

            vector<uint8_t> data (size, 0xFF);

            g_devices[0]->packets.emplace_back(data.data(), data.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.packetCount() == 1);
            REQUIRE(blob.data().size() == 0);        

        }

        SECTION("fetchData() removes idle words across packet boundaries") {

            size_t packetSize = WORD_SIZE;
            size_t size = PRELOAD + POSTLOAD + packetSize;

            vector<uint8_t> data (size - 1, 0xFF);
            vector<uint8_t> data2(size, 0x00);

            data2[0] = 0xFF;

            g_devices[0]->packets.emplace_back(data .data(), data .size());
            g_devices[0]->packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.packetCount() == 2);
            REQUIRE(blob.data().size() == WORD_SIZE);

        }

        SECTION("fetchData() removes idle words across fetch calls") {

            size_t packetSize = WORD_SIZE;
            size_t size = PRELOAD + POSTLOAD + packetSize;

            vector<uint8_t> data (size - 1, 0xFF);
            vector<uint8_t> data2(size + 1, 0x00);

            data2[PRELOAD] = 0xFF;

            g_devices[0]->packets.emplace_back(data.data(), data.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.packetCount() == 1);
            REQUIRE(blob.data().size() == 0);

            g_devices[0]->packets.emplace_back(data2.data(), data2.size());

            blob = handler.fetchData();

            REQUIRE(blob.packetCount() == 1);
            REQUIRE(blob.data().size() == WORD_SIZE);

            REQUIRE(blob.data()[0] == 0);

        }

        SECTION("fetchData() removes only the idle words") {

            size_t packetSize = WORD_SIZE * 3;
            size_t size = PRELOAD + POSTLOAD + packetSize;

            vector<uint8_t> data (size, 0x00);
            for(
                auto it = data.begin() + PRELOAD + WORD_SIZE; 
                it != data.begin() + PRELOAD + WORD_SIZE * 2;
                ++it
            ) {

                *it = 0xFF;

            }

            g_devices[0]->packets.emplace_back(data.data(), data.size());

            DataBlob blob = handler.fetchData();

            REQUIRE(blob.packetCount() == 1);
            REQUIRE(blob.data().size() == WORD_SIZE * 2);

            for(uint8_t i = 0; i < blob.data().size(); ++i) {

                blob.data()[i] = 0;

            }

        }

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