#include "PacketProcessor.h"

#include <numeric>

using namespace DAQCap;

using std::vector;

PacketProcessor::PacketProcessor()
    : lastPacket(nullptr) {}

DataBlob PacketProcessor::blobify(const vector<Packet> &packets) {

    DataBlob blob;

    // Record the number of packets
    blob.packets = packets.size();

    unpack(packets, blob);
    getWarnings(packets, blob);
    removeIdleWords(blob);

    return blob;

}

void PacketProcessor::reset() {

    lastPacket = nullptr;
    unfinishedWords.clear();

}

void PacketProcessor::unpack(
    const vector<Packet> &packets, 
    DataBlob &blob
) {

    // Put any unfinished words at the start of dataBuffer, and clear
    // unfinishedWords at the same time.
    std::swap(blob.dataBuffer, unfinishedWords);

    // This likely won't hold everything, but it will eliminate some extraneous
    // reallocations.
    // OPTIMIZATION -- It might be faster to accumulate packet sizes and
    //                 reserve the resulting size.
    blob.dataBuffer.reserve(packets.size() * Packet::WORD_SIZE);

    // Unpack packets into dataBuffer
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

    // Add any trailing unfinished word to unfinishedWords
    unfinishedWords.insert(
        unfinishedWords.end(),
        blob.dataBuffer.cend() - (blob.dataBuffer.size() % Packet::WORD_SIZE),
        blob.dataBuffer.cend()
    );

    // And erase it from dataBuffer
    blob.dataBuffer.erase(
        blob.dataBuffer.cend() - (blob.dataBuffer.size() % Packet::WORD_SIZE),
        blob.dataBuffer.cend()
    );

}

void PacketProcessor::getWarnings(
    const vector<Packet> &packets, 
    DataBlob &blob
) {

    // Start with the last packet we checked
    const Packet *prevPacket = lastPacket.get();

    // Check all the new packets sequentially
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

    // Store the last packet for next time
    if(prevPacket) {

        lastPacket = std::unique_ptr<Packet>(new Packet(*prevPacket));
        
    }

}

void PacketProcessor::removeIdleWords(DataBlob &blob) {

    // Now dataBuffer should start at the beginning of a word, so we can use 
    // that invariant to scan it for idle words.

    // Make a temporary vector and swap the dataBuffer into it
    std::vector<uint8_t> data;
    std::swap(data, blob.dataBuffer);

    // The dataBuffer is empty now, so let's reserve what we need
    blob.dataBuffer.reserve(data.size());

    // Scan through each word
    // NOTE: The unpacking logic guarantees that blob holds exactly an integer
    //       number of words, so we can trust that we won't go out of bounds.
    for(
        auto iter = data.cbegin();
        iter != data.cend();
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

            // If it isn't add it back to the dataBuffer
            // OPTIMIZATION -- Again, memcpy would be a bit faster, though less
            //                 so since we're only copying one word at a time.
            blob.dataBuffer.insert(
                blob.dataBuffer.end(),
                iter,
                iter + Packet::WORD_SIZE
            );

        }

    }

}