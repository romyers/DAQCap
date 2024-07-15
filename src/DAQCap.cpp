#include <DAQCap.h>

#include "NetworkInterface.h"
#include "DAQBlob.h"
#include "Packet.h"

#include <future>

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
      lastPacket(nullptr) {}

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

    bool exists = false;
    for(const shared_ptr<Device> d : netManager->getAllDevices()) {

        if(device->getName() == d->getName()) {

            exists = true;

            break;

        }

    }

    if(!exists) {

        throw std::runtime_error(
            "Device " + device->getName() + " does not exist."
        );

    }

    // Close the current session if there is one
    if(netManager->hasOpenSession()) {

        endSession();

    }

    netManager->startSession(device);

}

void SessionHandler::endSession() { 

    netManager->interrupt();
    netManager->endSession();

    unfinishedWords.clear();

    lastPacket = nullptr;
    
}

vector<shared_ptr<Device>> SessionHandler::getAllNetworkDevices(
    bool reload
) {

    if(reload) {

        networkDeviceCache.clear();

    }

    if(networkDeviceCache.empty()) {

        networkDeviceCache = netManager->getAllDevices();

    }

    return networkDeviceCache;

}

shared_ptr<Device> SessionHandler::getNetworkDevice(
    const string &name
) {

    if(name.empty()) return nullptr;

    // Check the cache first
    for(const shared_ptr<Device> device : networkDeviceCache) {

        if(device->getName() == name) return device;

    }

    // If the device wasn't in the cache, try reloading the device list
    vector<shared_ptr<Device>> devices;
    devices = getAllNetworkDevices(true);

    for(const shared_ptr<Device> device : devices) {

        if(device->getName() == name) return device;

    }

    return nullptr;

}

DataBlob SessionHandler::fetchData(
    std::chrono::milliseconds timeout, 
    int packetsToRead
) { 
    
    if(!netManager->hasOpenSession()) {

        throw std::logic_error(
            "Must start a session before fetching data."
        );

    }

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

        // This will force the netManager to return early
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

    DataBlob blob;

    if(packets.empty()) {

        return blob;

    }

    ///////////////////////////////////////////////////////////////////////////
    // Check for lost packets
    ///////////////////////////////////////////////////////////////////////////

    const Packet *prevPacket = lastPacket.get();
    for(const Packet &packet : packets) {

        if(prevPacket) {

            int gap = Packet::packetsBetween(*prevPacket, packet);

            if(gap != 0) {

                blob.warningsBuffer.push_back(
                    std::to_string(gap)
                        + " packets lost! Packet = "
                        + std::to_string(packet.getPacketNumber())
                        + ", Last = "
                        + std::to_string(prevPacket->getPacketNumber())
                );

            }

        }

        prevPacket = &packet;

    }

    if(prevPacket) {

        lastPacket = std::unique_ptr<Packet>(new Packet(*prevPacket));
        
    }

    ///////////////////////////////////////////////////////////////////////////
    // Package the data into the blob
    ///////////////////////////////////////////////////////////////////////////

    blob.packets = packets.size();

    vector<uint8_t> data;
    std::swap(data, unfinishedWords);

    // This likely won't hold everything, but it will eliminate some extraneous
    // reallocations.
    data.reserve(packets.size() * Packet::WORD_SIZE);

    for(const Packet &packet : packets) {

        data.insert(
            data.end(),
            packet.cbegin(),
            packet.cend()
        );

    }

    blob.dataBuffer.reserve(data.size());
    auto iter = data.cbegin();
    for(
        ;
        iter != data.cend() - (data.size() % Packet::WORD_SIZE);
        iter += Packet::WORD_SIZE
    ) {

        if(
            Packet::IDLE_WORD.empty() || // If so, there is no idle word
            !std::equal(
                iter,
                iter + Packet::WORD_SIZE,
                Packet::IDLE_WORD.cbegin()
            )
        ) {

            blob.dataBuffer.insert(
                blob.dataBuffer.end(),
                iter,
                iter + Packet::WORD_SIZE
            );

        }

    }

    unfinishedWords.insert(
        unfinishedWords.end(),
        iter,
        data.cend()
    );

    return blob;
    
}

void SessionHandler::interrupt() { 
    
    netManager->interrupt();
    
}