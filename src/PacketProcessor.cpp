#include "PacketProcessor.h"

#include <numeric>

using namespace DAQCap;

using std::vector;

PacketProcessor::PacketProcessor()
    : lastPacket(nullptr) {}

// TODO: We can extract lost packet checking and put it somewhere else
// TODO: Can we move idle word removal?
//         -- this is an operation on the unwound data, so it should be
//            easy. What we'll do is unwind everything into the blob, 
//            then separately remove idle words by:
//              -- iterate through blob data and move non-idles to a new
//                 vector
//              -- swap the new vector with blob data
DataBlob PacketProcessor::process(const vector<Packet> &packets) {

    DataBlob blob;

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

    // Put any unfinished words at the start of dataBuffer, and clear
    // unfinishedWords at the same time.
    std::swap(blob.dataBuffer, unfinishedWords);

    // This likely won't hold everything, but it will eliminate some extraneous
    // reallocations.
    // OPTIMIZATION -- It might be faster to accumulate packet sizes and
    //                 reserve the resulting size.
    blob.dataBuffer.reserve(packets.size() * Packet::WORD_SIZE);

    // Unwind packets into dataBuffer
    for(const Packet &packet : packets) {

        // OPTIMIZATION -- We could resize and memcpy into dataBuffer for
        //                 faster insertion. Fastest would likely be to
        //                 run through packets to get the total data size,
        //                 resize data to that size, and then memcpy in
        //                 each packet.
        blob.dataBuffer.insert(
            blob.dataBuffer.end(),
            packet.cbegin(),
            packet.cend()
        );

    }

    // Now data should start at the beginning of a word, so we can use that
    // invariant to scan it for idle words. Any word that isn't idle is
    // added to the blob.

    std::vector<uint8_t> data;
    data.reserve(blob.dataBuffer.size());

    // Scan through each word.
    auto iter = blob.dataBuffer.cbegin();
    for(
        ;
        iter != blob.dataBuffer.cend() - (blob.dataBuffer.size() % Packet::WORD_SIZE);
        iter += Packet::WORD_SIZE
    ) {

        // Check if the word is idle.
        if(
            Packet::IDLE_WORD.empty() || // If so, there is no idle word
            !std::equal(
                iter,
                iter + Packet::WORD_SIZE,
                Packet::IDLE_WORD.cbegin()
            )
        ) {

            // If it isn't add it to the blob.
            // OPTIMIZATION -- Again, memcpy would be a bit faster, though less
            //                 so since we're only copying one word at a time.
            data.insert(
                data.end(),
                iter,
                iter + Packet::WORD_SIZE
            );

        }

    }

    // Add the remainder from data to unfinishedWords
    unfinishedWords.insert(
        unfinishedWords.end(),
        iter,
        blob.dataBuffer.cend()
    );

    // Swap the cleaned data into the blob
    std::swap(blob.dataBuffer, data);

    return blob;

}