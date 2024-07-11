#include "NetworkInterface.h"

// TODO: We can expand the mock file into a full test file and get some extra
//       ways to get data out of the impl class.

using namespace DAQCap;

class MockListener: public Listener {


public:

    MockListener(const std::string &deviceName);
    ~MockListener();

    MockListener(const MockListener &other)            = delete;
    MockListener &operator=(const MockListener &other) = delete;

    void interrupt();
    std::vector<Packet> listen(int packetsToRead);

};

MockListener::MockListener(const std::string &deviceName) {

}

MockListener::~MockListener() {

}

void MockListener::interrupt() {

}

std::vector<Packet> MockListener::listen(int packetsToRead) {

    return std::vector<Packet>();

}

Listener *Listener::create(const std::string &deviceName) {

    return new MockListener(deviceName);

}