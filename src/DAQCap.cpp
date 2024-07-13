#include <DAQCap.h>

#include <vector>
#include <stdexcept>
#include <cstring>
#include <string>
#include <future>
#include <vector>
#include <cassert>

#include "NetworkInterface.h"

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
    DataBlob fetchData(
        std::chrono::milliseconds timeout, 
        int packetsToRead
    );

    void setIncludeIdleWords(bool discard);

private:

    Listener *listener;

    // Buffer for unfinished data words at the end of a packet
    std::vector<uint8_t> unfinishedWords;

    bool includingIdleWords = true;

    Packet lastPacket;

};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SessionHandler::SessionHandler(const Device &device)
    : impl(new SessionHandler_impl(device)) {}
SessionHandler::SessionHandler_impl::SessionHandler_impl(
    const Device &device
) : listener(Listener::create(device)) {

}


DataBlob SessionHandler::fetchData(
    std::chrono::milliseconds timeout, 
    int packetsToRead
) { return impl->fetchData(timeout, packetsToRead);}
DataBlob SessionHandler::SessionHandler_impl::fetchData(
    std::chrono::milliseconds timeout, 
    int packetsToRead
) {

    ///////////////////////////////////////////////////////////////////////////
    // Run a task that listens for packets
    ///////////////////////////////////////////////////////////////////////////

    // NOTE: This will create a new thread every time. Should optimization be
    //       necessary, we can use a packaged_task and assign it to a 
    //       persistent worker thread instead.
    auto future = std::async(
        std::launch::async, 
        [this, packetsToRead]() {

            return listener->listen(packetsToRead);

        }
    );

    ///////////////////////////////////////////////////////////////////////////
    // Wait and check if the task timed out
    ///////////////////////////////////////////////////////////////////////////

    // If we hit the timeout, interrupt the listener and report the timeout
    if(
        timeout != FOREVER &&
        future.wait_for(
            timeout
        ) == std::future_status::timeout
    ) {

        // This will force the listener to return early
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

    DataBlob blob;

    ///////////////////////////////////////////////////////////////////////////
    // Check for lost packets
    ///////////////////////////////////////////////////////////////////////////

    // Check across the boundary between fetchData calls
    if(lastPacket) {

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
    // Pack the data into the blob
    ///////////////////////////////////////////////////////////////////////////

    blob.packetCount = packets.size();

    std::vector<uint8_t> data;
    std::swap(data, unfinishedWords);

    // TODO: Optimization -- if we're careful about idle word checking, we can
    //       move directly from packet data to blob.data and avoid the extra
    //       move into data.

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
    blob.data.reserve(data.size());
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

            blob.data.insert(
                blob.data.end(),
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


SessionHandler::~SessionHandler() { if(impl) delete impl; }
SessionHandler::SessionHandler_impl::~SessionHandler_impl() {

    listener->interrupt();

    if(listener) delete listener;
    listener = nullptr;


}


void SessionHandler::setIncludeIdleWords(bool discard) {
    impl->setIncludeIdleWords(discard);
}
void SessionHandler::SessionHandler_impl::setIncludeIdleWords(bool discard) {

    includingIdleWords = discard;

}

void SessionHandler::interrupt() { impl->interrupt(); }
void SessionHandler::SessionHandler_impl::interrupt() {

    listener->interrupt();

}

std::vector<uint64_t> DataBlob::pack() {

    // The blob should not contain partial words
    assert(data.size() % Packet::wordSize() == 0);

    std::vector<uint64_t> packedData;
    packedData.reserve(data.size() / Packet::wordSize());
    
    // TODO: Test this
    //         -- Make sure the right bytes are in the right places
    //         -- Make sure we don't cut the last word off the end
    //         -- Look for more efficient ways to do this
    //              -- Only way I can think of is to memcpy then fix
    //                 endianness if there's a mismatch
    //         -- Make sure the endianness is correct -- check old
    //            code to verify this
    //         -- The decoder will need to pack raw data into words
    //            too. It might not be good to bury this in the
    //            DAQCap library
    //              -- We could consider merging the decode and
    //                 capture libraries though, creating a unified,
    //                 but modular library so that user can strip
    //                 out e.g. ethernet capture if they don't need it.

    // TODO: Faster ways to do this while staying agnostic to system
    //       endianness?
    for(
        size_t wordStart = 0; 
        wordStart < data.size(); 
        wordStart += Packet::wordSize()
    ) {

        int word = 0;
        
        for(size_t byte = 0; byte < Packet::wordSize(); ++byte) {

            wordStart |= data[wordStart + byte] << (8 * byte);

        }


    }

    return packedData;

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

std::vector<Device> SessionHandler::getNetworkDevices() {

    return getDevices();

}