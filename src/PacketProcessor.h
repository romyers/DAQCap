/**
 * @file PacketProcessor.h
 *
 * @brief Provides processing functionality for raw network data packets.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <DAQBlob.h>

#include "Packet.h"

#include <vector>
#include <memory>

namespace DAQCap {

    // TODO: Bundle this class in an existing .h?
    // TODO: Reflect semantically that it's more of a packet 'accumulator'

    class PacketProcessor {

    public:

        PacketProcessor();

        // TODO: What if we take an iterator range of packets instead of a
        //       vector?
        /**
         * @brief Processes a vector of packets.
         * 
         * @param packets The packets to process.
         * 
         * @return A blob containing the processed data.
         */
        DataBlob process(const std::vector<Packet> &packet);

        /*
        TODO: Try something like:

        // Just unpacks the packets
        vector<uint8_t> unpack(const std::vector<Packet> &packets);

        // Checks for lost packets
        std::vector<std::string> checkLostPackets(const std::vector<Packet> &packets);

        // Removes idle words (in-place)
        void removeIdleWords(std::vector<uint8_t> &data);

        */

    private:

        // The last packet processed
        std::unique_ptr<class Packet> lastPacket;

        // Buffer for unfinished data words at the end of a packet
        std::vector<uint8_t> unfinishedWords;

    };

}