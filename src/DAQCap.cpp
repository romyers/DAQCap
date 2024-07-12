#include <DAQCap.h>

#include <vector>
#include <stdexcept>
#include <cstring>
#include <string>
#include <future>

#include "NetworkInterface.h"
#include "WorkerThread.h"

#include <pcap.h>

using namespace DAQCap;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// PIMPL implementation class for SessionHandler
class SessionHandler::SessionHandler_impl {

public:

    SessionHandler_impl(const Device &device);
    ~SessionHandler_impl();

    void interrupt();
    DataBlob fetchPackets(int timeout, int packetsToRead);

private:

    Listener *listener;

    DAQThread::Worker<std::packaged_task<std::vector<Packet>()>> workerThread;

    int lastPacketNum = -1;

};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SessionHandler::SessionHandler_impl::SessionHandler_impl(
    const Device &device
) : listener(Listener::create(device)) {

}

SessionHandler::SessionHandler_impl::~SessionHandler_impl() {

    listener->interrupt();

    workerThread.terminate();
    workerThread.join();

    if(listener) delete listener;
    listener = nullptr;

}

void SessionHandler::SessionHandler_impl::interrupt() {

    listener->interrupt();

}

DataBlob SessionHandler::SessionHandler_impl::fetchPackets(
    int timeout,
    int packetsToRead
) {

    ///////////////////////////////////////////////////////////////////////////
    // Listen for packets with timeout using C++ async tools
    ///////////////////////////////////////////////////////////////////////////

    // Create a packaged_task out of the listen() call and get a future from it
    std::packaged_task<std::vector<Packet>()> task([this, packetsToRead]() {

        return listener->listen(packetsToRead);

    });
    std::future<std::vector<Packet>> future = task.get_future();

    // Add the task to the worker thread
    workerThread.assignTask(std::move(task));

    // If we hit the timeout, interrupt the listener and report the timeout
    if(
        timeout != NO_LIMIT &&
        future.wait_for(
            std::chrono::milliseconds(timeout)
        ) == std::future_status::timeout
    ) {

        interrupt();
        future.wait();

        throw timeout_exception("DAQCap::SessionHandler::fetchPackets() timed out.");

    }

    // Wait for the operation to finish and get the result
    future.wait();

    std::vector<Packet> packets;
    try {

        packets = future.get();

    } catch(const std::exception &e) {

        throw std::runtime_error(
            std::string("Failed to fetch packets: ") + e.what()
        );

    }

    // TODO: Signal to the user that we timed out, if we timed out.
    //       Probably we want to look for exceptions thrown by the listener
    //       if we got -1 or -2 out of pcap_dispatch

    ///////////////////////////////////////////////////////////////////////////
    // Pack the data into the blob
    ///////////////////////////////////////////////////////////////////////////

    // TODO: Check packet numbers for continuity
    // TODO: Exclude idle words
    // TODO: Make sure we can trust data blobs to hold exactly an integral 
    //       number of words
    //         -- Maybe we split packets into words?

    DataBlob blob;

    // Unwind the packets into the blob
    blob.packetNumbers.reserve(packets.size());
    for(const Packet &packet : packets) {

        blob.packetNumbers.push_back(packet.getPacketNumber());

        blob.data.insert(
            blob.data.end(), 
            packet.cbegin(),
            packet.cend()
        );

    }

    return blob;

    // TODO: We abstracted out info about the network interface (i.e. pcap).
    //       We should also abstract out info about the packet format. So
    //       e.g. if someone changes the DAQ interface, the packet format is
    //       in one easy to find place so it's easy to update the library.
    //       That packet format should also include the word size, and all
    //       other code that uses the word size should get it from there.
    //         -- This will do the same things the Signal class does, so
    //            we should couple them together. Signal is all about
    //            data format.
    //         -- Packet is a good place to put all this.
    //         -- The data format class should be pure virtual.
    //         -- Perhaps we replace DataBlob with Packet.
    //         -- Maybe it makes sense to decode everything into signals
    //            right in the DAQCap module. But we still need to be able
    //            to write raw data files.

    // std::vector<unsigned char> idleWord(wordSize, 0xFF);
    // for(const Packet &packet : g_packetBuffer) {

        ///////////////////////////////////////////////////////////////////////

        /*
        for(
            int iter = DATA_START; 
            iter < packet.header->len - 4; 
            iter += wordSize
        ) {

            // TODO: Check that I have this condition the right way around
            if(
                std::memcmp(
                    packet.data + iter, idleWord.data(), wordSize
                ) != 0
            ) {

                blob.data.insert(
                    blob.data.end(), 
                    packet.data + iter, 
                    packet.data + iter + wordSize
                );

            }

        }
        */

        ///////////////////////////////////////////////////////////////////////

    // }

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int DataBlob::countPackets() const {

    return packetNumbers.size();

}

// TODO: Get rid of this and remove pcap.h header
std::vector<Device> SessionHandler::getDeviceList() {

    return getDevices();

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SessionHandler::SessionHandler(const Device &device)
    : impl(new SessionHandler_impl(device)) {}

SessionHandler::~SessionHandler() {
    
    if(impl) delete impl;
    impl = nullptr;
    
}

void SessionHandler::interrupt() { 

    impl->interrupt(); 

}

DataBlob SessionHandler::fetchPackets(int timeout, int packetsToRead) { 

    return impl->fetchPackets(timeout, packetsToRead); 

}