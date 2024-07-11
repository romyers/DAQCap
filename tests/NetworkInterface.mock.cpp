#include "NetworkInterface.h"

// TODO: We can expand the mock file into a full test file and get some extra
//       ways to get data out of the impl class.

using namespace DAQCap;

class Listener::Listener_impl {

public:

    Listener_impl(const std::string &deviceName);

    void interrupt();

    std::vector<Packet> listen(int packetsToRead);

private:

    bool interrupted = false;

};

Listener::Listener_impl::Listener_impl(const std::string &deviceName) {}

void Listener::Listener_impl::interrupt() {

    // TODO: Some way to test that this happened

    interrupted = true;

}

std::vector<Packet> Listener::Listener_impl::listen(int packetsToRead) { 

    std::vector<Packet> packets;

    // TODO: Stuff a vector of packets with some explicit data

    return packets; 

}

Listener::Listener(const std::string &deviceName) 
    : impl(new Listener_impl(deviceName)) {}

Listener::~Listener() {
    
    if(impl) delete impl;
    impl = nullptr;

}

void Listener::interrupt() { 

    impl->interrupt(); 

}

std::vector<Packet> Listener::listen(int packetsToRead) { 

    return impl->listen(packetsToRead); 

}