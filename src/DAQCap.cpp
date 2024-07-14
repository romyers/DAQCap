#include <DAQCap.h>
#include "DAQSession_impl.h"

#include "NetworkInterface.h"
#include "DAQBlob.h"

#include <vector>
#include <stdexcept>
#include <cstring>
#include <string>
#include <future>
#include <vector>

#include <pcap.h>

using namespace DAQCap;

SessionHandler::SessionHandler() 
    : impl(new SessionHandler::SessionHandler_impl()) {}

SessionHandler::~SessionHandler() { 
    
    if(impl) delete impl; 
    
}

void SessionHandler::startSession(const Device &device) {

    impl->startSession(device);

}

void SessionHandler::endSession() { 
    
    impl->endSession(); 
    
}

std::vector<Device> SessionHandler::getAllNetworkDevices() {

    return impl->getAllNetworkDevices();

}

Device SessionHandler::getNetworkDevice(const std::string &name) {

    return impl->getNetworkDevice(name);

}

DataBlob SessionHandler::fetchData(
    std::chrono::milliseconds timeout, 
    int packetsToRead
) { 
    
    return impl->fetchData(timeout, packetsToRead);
    
}


void SessionHandler::includeIdleWords(bool discard) {

    impl->includeIdleWords(discard);

}


void SessionHandler::interrupt() { 
    
    impl->interrupt(); 
    
}

int DataBlob::packetCount() {

    return packets;

}