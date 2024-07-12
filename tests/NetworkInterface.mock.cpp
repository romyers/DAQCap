#include "NetworkInterface.h"

#include <cstring>
#include <thread>

// TODO: We can expand the mock file into a full test file and get some extra
//       ways to get data out of the MockListener class.

using namespace DAQCap;

std::vector<Device> DAQCap::getDevices() {

    std::vector<Device> devices;

    devices.push_back({ "MockDeviceName", "MockDeviceDescription" });
    devices.push_back({ "MockDevice2Name", "MockDevice2Description" });

    return devices;

}

class MockListener: public Listener {


public:

    MockListener(const Device &device);
    ~MockListener();

    MockListener(const MockListener &other)            = delete;
    MockListener &operator=(const MockListener &other) = delete;

    void interrupt();
    std::vector<Packet> listen(int packetsToRead);

private:

    bool interrupted = false;

};

MockListener::MockListener(const Device &device) {

}

MockListener::~MockListener() {

}

void MockListener::interrupt() {

    interrupted = true;

}

std::vector<Packet> MockListener::listen(int packetsToRead) {

    std::vector<Packet> packets;

    if(interrupted) {

        interrupted = false;
        return packets;

    }

    for(int i = 0; i < std::min(packetsToRead, 100); ++i) {

        int size = 14 + 256 + 4;

        unsigned char *data = new unsigned char[size];

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

Listener *Listener::create(const Device &device) {

    return new MockListener(device);

}