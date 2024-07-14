#include <DAQCap.h>
#include "DAQSession_impl.h"

#include "NetworkInterface.h"
#include "DAQBlob.h"

#include <future>

using namespace DAQCap;

SessionHandler::SessionHandler_impl::SessionHandler_impl()
    : netManager(NetworkManager::create()) {}

SessionHandler::SessionHandler_impl::~SessionHandler_impl() {

    endSession();

    delete netManager;
    netManager = nullptr;

}

void SessionHandler::SessionHandler_impl::endSession() {

    netManager->interrupt();
    netManager->endSession();

    unfinishedWords.clear();

    lastPacket = Packet();

}

void SessionHandler::SessionHandler_impl::includeIdleWords(bool discard) {

    includingIdleWords = discard;

}

void SessionHandler::SessionHandler_impl::interrupt() {

    if(!netManager) return;

    netManager->interrupt();

}

std::vector<Device> SessionHandler::SessionHandler_impl::getAllNetworkDevices() {

    if(networkDeviceCache.empty()) {

        networkDeviceCache = netManager->getAllDevices();

    }

    return networkDeviceCache;

}

Device SessionHandler::SessionHandler_impl::getNetworkDevice(
    const std::string &name
) {

    if(name.empty()) return Device();

    std::vector<Device> devices;
    
    try {

        devices = getAllNetworkDevices();

    } catch(...) {

        return Device();

    }

    for(const Device &device : devices) {

        if(device.getName() == name) return device;

    }

    return Device();

}
    
void SessionHandler::SessionHandler_impl::startSession(const Device &device) {

    // Make sure the device exists
    if(!device) {

        throw std::runtime_error(
            "Cannot start a session with an empty device."
        );

    }

    for(const Device &d : netManager->getAllDevices()) {

        if(device.getName() == d.getName()) {

            break;

        }

        throw std::runtime_error(
            "Device " + device.getName() + " does not exist."
        );

    }

    // Close the current session if there is one
    if(netManager->hasOpenSession()) {

        endSession();

    }

    netManager->startSession(device);

}

DataBlob SessionHandler::SessionHandler_impl::fetchData(
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

    std::vector<Packet> packets;
    try {

        packets = future.get();

    } catch(const std::exception &e) {

        throw std::runtime_error(
            std::string("Failed to fetch packets: ") + e.what()
        );

    }

    if(packets.empty()) {

        return DataBlob();

    }

    ///////////////////////////////////////////////////////////////////////////
    // Check for lost packets
    ///////////////////////////////////////////////////////////////////////////

    DataBlob blob;

    // Check across the boundary between fetchData calls
    if(lastPacket) {

        int gap = Packet::packetsBetween(lastPacket, packets.front());

        if(gap != 0) {

            blob.warningsBuffer.push_back(
                std::to_string(gap)
                    + " packets lost! Packet = "
                    + std::to_string(packets.front().getPacketNumber())
                    + ", Last = "
                    + std::to_string(lastPacket.getPacketNumber())
            );

        }

    }
    lastPacket = packets.back();

    // Check within the packets fetched in this call
    for(int i = 1; i < packets.size(); ++i) {

        int gap = Packet::packetsBetween(packets[i - 1], packets[i]);

        if(gap != 0) {

            blob.warningsBuffer.push_back(
                std::to_string(gap)
                    + " packets lost! Packet = "
                    + std::to_string(packets[i].getPacketNumber())
                    + ", Last = "
                    + std::to_string(packets[i - 1].getPacketNumber())
            );

        }

    }

    ///////////////////////////////////////////////////////////////////////////
    // Package the data into the blob
    ///////////////////////////////////////////////////////////////////////////

    blob.packets = packets.size();

    std::vector<uint8_t> data;
    std::swap(data, unfinishedWords);

    // This likely won't hold everything, but it will eliminate some extraneous
    // reallocations.
    data.reserve(packets.size() * Packet::wordSize());

    for(const Packet &packet : packets) {

        data.insert(
            data.end(),
            packet.cbegin(),
            packet.cend()
        );

    }

    // TODO: Test very carefully that this loop terminates in the right place
    //       and leaves iter where it needs to be for the following insert call
    blob.dataBuffer.reserve(data.size());
    auto iter = data.begin();
    for(
        ;
        iter != data.end() - data.size() % Packet::wordSize();
        iter += Packet::wordSize()
    ) {

        // TODO: Test idle word removal
        if(
            includingIdleWords ||
            Packet::idleWord().empty() || // If so, there is no idle word
            !std::equal(
                iter,
                iter + Packet::wordSize(),
                Packet::idleWord().cbegin()
            )
        ) {

            blob.dataBuffer.insert(
                blob.dataBuffer.end(),
                std::make_move_iterator(iter),
                std::make_move_iterator(iter + Packet::wordSize())
            );

        }

    }

    unfinishedWords.insert(
        unfinishedWords.end(),
        std::make_move_iterator(iter),
        std::make_move_iterator(data.end())
    );

    return blob;

}