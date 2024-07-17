#include <DAQCap.h>

#include "NetworkInterface.h"
#include "DAQBlob.h"
#include "Packet.h"
#include "PacketProcessor.h"

#include <future>
#include <algorithm>

using std::vector;
using std::string;

using std::shared_ptr;
using std::unique_ptr;

using namespace DAQCap;

const int DAQCap::ALL_PACKETS = -1;

const std::chrono::milliseconds DAQCap::FOREVER 
    = std::chrono::milliseconds(-1);

SessionHandler::SessionHandler() 
    : netManager(NetworkManager::create()),
      packetProcessor(nullptr) {}

SessionHandler::~SessionHandler() { 

    endSession();

    netManager = nullptr;
    
}

void SessionHandler::startSession(const shared_ptr<Device> device) {

    // Make sure the device exists
    if(!device) {

        throw std::runtime_error(
            "Cannot start a session with an empty device."
        );

    }

    if(!getNetworkDevice(device->getName())) {

        throw std::runtime_error(
            "Device " + device->getName() + " does not exist."
        );

    }

    // End any existing session
    endSession();

    packetProcessor.reset(new PacketProcessor());

    netManager->startSession(device);

}

void SessionHandler::endSession() { 

    interrupt();
    netManager->endSession();
    
}

vector<shared_ptr<Device>> SessionHandler::getAllNetworkDevices() {

    return netManager->getAllDevices();

}

shared_ptr<Device> SessionHandler::getNetworkDevice(
    const string &name
) {

    vector<shared_ptr<Device>> devices = getAllNetworkDevices();

    for(const shared_ptr<Device> device : devices) {

        if(device->getName() == name) return device;

    }

    return nullptr;

}

DataBlob SessionHandler::fetchData(
    std::chrono::milliseconds timeout, 
    int packetsToRead
) { 

    DataBlob blob;

    ///////////////////////////////////////////////////////////////////////////
    // Run a task that listens for packets
    ///////////////////////////////////////////////////////////////////////////

    auto future = std::async(
        std::launch::async, 
        [this, packetsToRead]() {

            return netManager->fetchPackets(packetsToRead);

        }
    );

    ///////////////////////////////////////////////////////////////////////////
    // Wait and check if the task timed out
    ///////////////////////////////////////////////////////////////////////////

    if(
        timeout != FOREVER &&
        future.wait_for(
            timeout
        ) == std::future_status::timeout
    ) {

        // This should force the netManager to return early
        interrupt();

        throw timeout_exception(
            "DAQCap::SessionHandler::fetchData() timed out."
        );

        // NOTE: Future will block until completion when it goes out of scope,
        //       so we don't need to explicitly wait for it

    }

    ///////////////////////////////////////////////////////////////////////////
    // Get the result
    ///////////////////////////////////////////////////////////////////////////

    vector<Packet> packets;
    try {

        packets = future.get();

    } catch(const std::exception &e) {

        throw std::runtime_error(
            string("Failed to fetch packets: ") + e.what()
        );

    }

    return packetProcessor->process(packets);
    
}

void SessionHandler::interrupt() { 
    
    netManager->interrupt();
    
}