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
    DataBlob fetchPackets(int timeout, int packetsToRead);

    void startDiscardingIdleWords();
    void stopDiscardingIdleWords();

private:

    Listener *listener;

    DAQThread::Worker<std::packaged_task<std::vector<Packet>()>> workerThread;

    // Buffer for unfinished data words at the end of a packet
    std::deque<unsigned char> unfinishedWords;

    bool isDiscardingIdleWords = true;

};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SessionHandler::SessionHandler_impl::SessionHandler_impl(
    const Device &device
) : listener(Listener::create(device)) {

}

DataBlob SessionHandler::SessionHandler_impl::fetchPackets(
    int timeout,
    int packetsToRead
) {

    // TODO: Extract timeout logic somewhere
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

    ///////////////////////////////////////////////////////////////////////////
    // Pack the data into the blob
    ///////////////////////////////////////////////////////////////////////////

    // TODO: Check packet numbers for continuity and report gaps

    DataBlob blob;

    std::deque<unsigned char> data;
    std::swap(data, unfinishedWords);

    for(const Packet &packet : packets) {

        data.insert(
            data.end(),
            packet.cbegin(),
            packet.cend()
        );

    }

    blob.packetCount = packets.size();
    blob.data.reserve(data.size());

    while(data.size() >= Packet::WORD_SIZE) {

        if(
            !isDiscardingIdleWords ||
            !std::equal(
                data.cbegin(),
                data.cbegin() + Packet::WORD_SIZE,
                Packet::IDLE_WORD.cbegin()
            )
        ) {

            blob.data.insert(
                blob.data.end(),
                std::make_move_iterator(data.begin()),
                std::make_move_iterator(data.begin() + Packet::WORD_SIZE)
            );

        }

        // TODO: Is it faster to just loop and pop_front? I think erase might
        //       be linear even in this case.
        // TODO: Or maybe it's better to push everything to the front instead
        //       of the back and then erase from the back?
        data.erase(data.begin(), data.begin() + Packet::WORD_SIZE);

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
    //         -- Perhaps we replace DataBlob with Packet.
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

void SessionHandler::SessionHandler_impl::startDiscardingIdleWords() {

    isDiscardingIdleWords = true;

}

void SessionHandler::SessionHandler_impl::stopDiscardingIdleWords() {

    isDiscardingIdleWords = false;

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

DataBlob SessionHandler::fetchPackets(int timeout, int packetsToRead) { 

    return impl->fetchPackets(timeout, packetsToRead); 

}

void SessionHandler::startDiscardingIdleWords() {

    impl->startDiscardingIdleWords();

}

void SessionHandler::stopDiscardingIdleWords() {

    impl->stopDiscardingIdleWords();

}