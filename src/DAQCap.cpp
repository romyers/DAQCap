#include <DAQCap.h>

#include <vector>
#include <stdexcept>
#include <cstring>
#include <string>
#include <future>
#include <deque>

#include "NetworkInterface.h"
#include "WorkerThread.h"

#include <pcap.h>

#include <iostream>

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
    DataBlob fetchData(
        int timeout, 
        int packetsToRead
    );

    void setIncludeIdleWords(bool discard);

private:

    Listener *listener;

    DAQThread::Worker<std::packaged_task<std::vector<Packet>()>> workerThread;

    // Buffer for unfinished data words at the end of a packet
    std::deque<unsigned char> unfinishedWords;

    bool includingIdleWords = true;

    Packet lastPacket;

};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SessionHandler::SessionHandler_impl::SessionHandler_impl(
    const Device &device
) : listener(Listener::create(device)) {

}

DataBlob SessionHandler::SessionHandler_impl::fetchData(
    int timeout, 
    int packetsToRead
) {

    ///////////////////////////////////////////////////////////////////////////
    // Run a task that listens for packets
    ///////////////////////////////////////////////////////////////////////////

    // Create a packaged_task out of the listen() call
    std::packaged_task<std::vector<Packet>()> task([this, packetsToRead]() {

        return listener->listen(packetsToRead);

    });
    std::future<std::vector<Packet>> future = task.get_future();

    // Add the task to the worker thread
    workerThread.assignTask(std::move(task));

    // TODO: Extract timeout logic somewhere
    ///////////////////////////////////////////////////////////////////////////
    // Wait and check if the task timed out
    ///////////////////////////////////////////////////////////////////////////

    // If we hit the timeout, interrupt the listener and report the timeout
    if(
        timeout != NO_LIMIT &&
        future.wait_for(
            std::chrono::milliseconds(timeout)
        ) == std::future_status::timeout
    ) {

        interrupt();
        future.wait();

        throw timeout_exception("DAQCap::SessionHandler::fetchData() timed out.");

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

    // TODO: Check packet numbers for continuity and report gaps
    //         -- The troublesome part is figuring out how to emit the relevant
    //            data to the user when there are gaps

    DataBlob blob;

    blob.packetCount = packets.size();

    std::deque<unsigned char> data;
    std::swap(data, unfinishedWords);

    ///////////////////////////////////////////////////////////////////////////
    // Check for lost packets
    ///////////////////////////////////////////////////////////////////////////

    // Check across the boundary between fetchData calls
    if(!lastPacket.isNull()) {

        int gap = Packet::packetsBetween(lastPacket, packets.front());

        if(gap != 0) {

            blob.warnings.push_back(
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

            blob.warnings.push_back(
                std::to_string(gap)
                    + " packets lost! Packet = "
                    + std::to_string(packets[i].getPacketNumber())
                    + ", Last = "
                    + std::to_string(packets[i - 1].getPacketNumber())
            );

        }

    }

    ///////////////////////////////////////////////////////////////////////////
    // Pack the data into the output buffer
    ///////////////////////////////////////////////////////////////////////////

    for(int i = 0; i < packets.size(); ++i) {

        data.insert(
            data.end(),
            packets[i].cbegin(),
            packets[i].cend()
        );

    }

    for(const Packet &packet : packets) {

        data.insert(
            data.end(),
            packet.cbegin(),
            packet.cend()
        );

    }

    blob.data.reserve(data.size());
    while(data.size() >= Packet::wordSize()) {

        if(
            includingIdleWords ||
            Packet::idleWord().empty() || // If so, there is no idle word
            !std::equal(
                data.cbegin(),
                data.cbegin() + Packet::wordSize(),
                Packet::idleWord().cbegin()
            )
        ) {

            blob.data.insert(
                blob.data.end(),
                std::make_move_iterator(data.begin()),
                std::make_move_iterator(data.begin() + Packet::wordSize())
            );

        }

        // TODO: Is it faster to just loop and pop_front? I think erase might
        //       be linear in data.size() even in this case.
        data.erase(data.begin(), data.begin() + Packet::wordSize());

    }

    std::swap(unfinishedWords, data);

    return blob;

    // TODO: We abstracted out info about the network interface (i.e. pcap).
    //       We should also abstract out info about the packet format. So
    //       e.g. if someone changes the DAQ interface, the packet format is
    //       in one easy to find place so it's easy to update the library.
    //       That packet format should also include the word size, and all
    //       other code that uses the word size should get it from there.
    //         -- This will do the same things the Signal class does, so
    //            we should couple them together. Signal is all about
    //            data format. It would be good to keep all the data format
    //            details together.
    //         -- Packet is a good place to put all this.
    //         -- The data format class should be pure virtual.
    //         -- Maybe it makes sense to decode everything into signals
    //            right in the DAQCap module. But we still need to be able
    //            to write raw data files.

}

SessionHandler::SessionHandler_impl::~SessionHandler_impl() {

    listener->interrupt();

    workerThread.terminate();
    workerThread.join();

    if(listener) delete listener;
    listener = nullptr;

}

void SessionHandler::SessionHandler_impl::setIncludeIdleWords(bool discard) {

    includingIdleWords = discard;

}

void SessionHandler::SessionHandler_impl::interrupt() {

    listener->interrupt();

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

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

DataBlob SessionHandler::fetchData(
    int timeout, 
    int packetsToRead
) { 

    return impl->fetchData(timeout, packetsToRead);

}

void SessionHandler::setIncludeIdleWords(bool discard) {

    impl->setIncludeIdleWords(discard);

}