#include <DAQCap.h>

#include <vector>
#include <stdexcept>
#include <cstring>
#include <string>
#include <future>

#include "NetworkInterface.h"
#include "WorkerThread.h"

#include <pcap.h>

// TODO: Try to get rid of throws

// TODO: Try to make Device opaque?

using namespace DAQCap;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class SessionHandler::SessionHandler_impl {

public:

    SessionHandler_impl(const Device &device);
    ~SessionHandler_impl();

    void interrupt();
    DataBlob fetchPackets(int timeout, int packetsToRead);

private:

    Listener *listener;

    DAQThread::Worker<std::packaged_task<std::vector<Packet>()>> workerThread;

};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SessionHandler::SessionHandler_impl::SessionHandler_impl(
    const Device &device
) : listener(Listener::create(device)) {

}

SessionHandler::SessionHandler_impl::~SessionHandler_impl() {

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

    // TODO: We can improve this by posting our task to a persistent thread.
    //       We can close and join the thread in the destructor for 
    //       SessionHandler_impl. Use:
    //       https://en.cppreference.com/w/cpp/thread/condition_variable
    //       https://stackoverflow.com/questions/53014805/add-a-stdpackaged-task-to-an-existing-thread
    //       https://stackoverflow.com/questions/35827459/assigning-a-new-task-to-a-thread-after-the-thread-completes-in-c

    // Create a packaged_task out of the listen() call and get a future from it
    std::packaged_task<std::vector<Packet>()> task([this, packetsToRead]() {

        return listener->listen(packetsToRead);

    });
    std::future<std::vector<Packet>> future = task.get_future();

    // Add the task to the worker thread
    workerThread.assignTask(std::move(task));

    // If we hit the timeout, interrupt the listener
    if(
        timeout != NO_LIMIT &&
        future.wait_for(
            std::chrono::milliseconds(timeout)
        ) == std::future_status::timeout
    ) {

        interrupt();

    }

    // TODO: When we get a thread pool going, replace with future.wait()
    //         -- A thread pool should clean up the multithreading logic in
    //            DAQManager too.
    //         -- Thread pool should have an option to force something to
    //            start immediately. When this option occurs, we make a new
    //            thread if existing threads are occupied. Then when a thread
    //            frees up, we get rid of threads until we're back to the
    //            hardware limit.
    // Wait for the operation to finish and get the result
    future.wait();
    std::vector<Packet> packets = future.get();

    // TODO: Signal to the user that we timed out, if we timed out.
    //       Probably we want to look for exceptions thrown by the listener
    //       if we got -1 or -2 out of pcap_dispatch

    ///////////////////////////////////////////////////////////////////////////
    // Pack the data into the blob
    ///////////////////////////////////////////////////////////////////////////

    DataBlob blob;

    // Strip out preload/postload bytes from each packet's data and store in the blob
    blob.packetNumbers.reserve(packets.size());
    for(const Packet &packet : packets) {

        // 14 bytes of preload
        const unsigned char *dataStart = packet.data + 14; 

        // 4 bytes of postload
        const unsigned char *dataEnd = packet.data + packet.size - 4; 

        // TODO: It might be better to maintain the separation between each
        //       packet's data in the blob, rather than concatenating it all
        //       together. It's conceivable that clients of this library may
        //       want to inspect individual packets. But really I want to make
        //       sure that everything they might need individual packets for
        //       is handled internally in the library.
        //         -- On that note, we may want to avoid exposing packet
        //            numbers to client code.
        //         -- Well, either avoid exposing packet numbers or preserve
        //            the distinction between data from different packets.

        // Extract packet number from the packet and store it in the blob
        int packetNum = (int)(*(packet.data + packet.size - 2) * 256);
        packetNum += (int)(*(packet.data + packet.size - 1));

        blob.packetNumbers.push_back(packetNum);

        // Strip out preload/postload bytes from each packet's data and store
        // the rest in the blob
        blob.data.insert(
            blob.data.end(), 
            dataStart,
            dataEnd
        );

    }

    return blob;

    // std::vector<unsigned char> idleWord(wordSize, 0xFF);
    // for(const Packet &packet : g_packetBuffer) {

        /*

        if(checkPackets) {

            if(lastPacket != -1) {

                if(packetNum != (lastPacket + 1) % 65536) {

                    int missingPackets 
                        = (packetNum - (lastPacket + 1)) % 65536;

                    blob.errorMessages.push_back(
                        std::to_string(missingPackets) 
                            + " packets lost! Packet = "
                            + std::to_string(packetNum) 
                            + ", Last = " 
                            + std::to_string(lastPacket)
                    );

                    blob.lostPackets += missingPackets;

                }

            }

        }

        // TODO: If we pull out validation, how do we handle updating this?
        lastPacket = packetNum;

        */

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