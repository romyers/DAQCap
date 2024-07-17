#include "PacketProcessor.h"

#include <numeric>

using namespace DAQCap;

using std::vector;

PacketProcessor::PacketProcessor()
    : lastPacket(nullptr) {}

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

    vector<uint8_t> data;

    // Put any unfinished words at the start of data, and clear unfinishedWords
    // at the same time.
    std::swap(data, unfinishedWords);

    // This likely won't hold everything, but it will eliminate some extraneous
    // reallocations.
    // OPTIMIZATION -- It might be faster to accumulate packet sizes and
    //                 reserve the resulting size.
    data.reserve(packets.size() * Packet::WORD_SIZE);

    // Unwind packets into data
    for(const Packet &packet : packets) {

        // OPTIMIZATION -- We could resize and memcpy into data.data() for
        //                 faster insertion. Fastest would likely be to
        //                 run through packets to get the total data size,
        //                 resize data to that size, and then memcpy in
        //                 each packet.
        data.insert(
            data.end(),
            packet.cbegin(),
            packet.cend()
        );

    }

    // Now data should start at the beginning of a word, so we can use that
    // invariant to scan it for idle words. Any word that isn't idle is
    // added to the blob.

    blob.dataBuffer.reserve(data.size());

    // Scan through each word.
    auto iter = data.cbegin();
    for(
        ;
        iter != data.cend() - (data.size() % Packet::WORD_SIZE);
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
            blob.dataBuffer.insert(
                blob.dataBuffer.end(),
                iter,
                iter + Packet::WORD_SIZE
            );

        }

    }

    // Add the remainder from data to unfinishedWords
    unfinishedWords.insert(
        unfinishedWords.end(),
        iter,
        data.cend()
    );

    return blob;

}