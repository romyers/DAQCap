#include "PacketProcessor.h"

#include <numeric>

using namespace DAQCap;

using std::vector;

PacketProcessor::PacketProcessor()
    : lastPacket(nullptr) {}

<<<<<<< HEAD
// TODO: We can extract lost packet checking and put it somewhere else
// TODO: Can we move idle word removal?
//         -- this is an operation on the unwound data, so it should be
//            easy. What we'll do is unwind everything into the blob, 
//            then separately remove idle words by:
//              -- iterate through blob data and move non-idles to a new
//                 vector
//              -- swap the new vector with blob data
=======
>>>>>>> 52957ccffdd945a3c1f47889ce9ed85e44a24a9a
DataBlob PacketProcessor::process(const vector<Packet> &packets) {

    DataBlob blob;

    ///////////////////////////////////////////////////////////////////////////
<<<<<<< HEAD
    // Unpack packets
    ///////////////////////////////////////////////////////////////////////////

    // Record the number of packets
    blob.packets = packets.size();

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

    ///////////////////////////////////////////////////////////////////////////
    // Check for lost packets
    ///////////////////////////////////////////////////////////////////////////

    // Start with the last packet we checked
    const Packet *prevPacket = lastPacket.get();

    // Check all the new packets sequentially
=======
    // Check for lost packets
    ///////////////////////////////////////////////////////////////////////////

    const Packet *prevPacket = lastPacket.get();
>>>>>>> 52957ccffdd945a3c1f47889ce9ed85e44a24a9a
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

<<<<<<< HEAD
    // Store the last packet for next time
=======
>>>>>>> 52957ccffdd945a3c1f47889ce9ed85e44a24a9a
    if(prevPacket) {

        lastPacket = std::unique_ptr<Packet>(new Packet(*prevPacket));
        
    }

    ///////////////////////////////////////////////////////////////////////////
    // Package the data into the blob
    ///////////////////////////////////////////////////////////////////////////

<<<<<<< HEAD
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
=======
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
>>>>>>> 52957ccffdd945a3c1f47889ce9ed85e44a24a9a
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

<<<<<<< HEAD
            // If it isn't add it back to the dataBuffer
=======
            // If it isn't add it to the blob.
>>>>>>> 52957ccffdd945a3c1f47889ce9ed85e44a24a9a
            // OPTIMIZATION -- Again, memcpy would be a bit faster, though less
            //                 so since we're only copying one word at a time.
            blob.dataBuffer.insert(
                blob.dataBuffer.end(),
                iter,
                iter + Packet::WORD_SIZE
            );

        }

    }

<<<<<<< HEAD
=======
    // Add the remainder from data to unfinishedWords
    unfinishedWords.insert(
        unfinishedWords.end(),
        iter,
        data.cend()
    );

>>>>>>> 52957ccffdd945a3c1f47889ce9ed85e44a24a9a
    return blob;

}