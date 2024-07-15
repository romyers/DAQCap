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

using std::vector;
using std::string;

using std::shared_ptr;

using namespace DAQCap;

const int DAQCap::ALL_PACKETS = -1;

const std::chrono::milliseconds DAQCap::FOREVER 
    = std::chrono::milliseconds(-1);

SessionHandler::SessionHandler() 
    : impl(new SessionHandler::SessionHandler_impl()) {}

SessionHandler::~SessionHandler() { 
    
    if(impl) delete impl; 
    
}

void SessionHandler::startSession(const shared_ptr<Device> device) {

    impl->startSession(device);

}

void SessionHandler::endSession() { 
    
    impl->endSession(); 
    
}

vector<shared_ptr<Device>> SessionHandler::getAllNetworkDevices(
    bool reload
) {

    return impl->getAllNetworkDevices(reload);

}

shared_ptr<Device> SessionHandler::getNetworkDevice(
    const string &name
) {

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