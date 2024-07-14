/**
 * @file DAQBlob.h
 *
 * @brief Packages captured data.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <vector>
#include <string>

namespace DAQCap {

    // OPTIMIZATION: If performance matters, we can return DataBlob values
    //               by const reference instead of by value, at the risk of a 
    //               more complex interface.
    /**
     * @brief Represents a blob of data fetched from a network device.
     * 
     * @note DataBlobs contain exactly an integral number of words.
     */
    class DataBlob {

    public:

        DataBlob() = default;

        /**
         * @brief Gets number of packets in the data blob.
         */
        int packetCount();

        /**
         * @brief Gets data fetched from the network device.
         */
        std::vector<uint8_t> data();

        /**
         * @brief Gets warnings that were generated during the fetch.
         */
        std::vector<std::string> warnings();

        /**
         * @brief Packs the data blob into a vector of 64-bit integers.
         */
        std::vector<uint64_t> pack();

    private:

        int packets = 0;

        std::vector<uint8_t> dataBuffer;

        std::vector<std::string> warningsBuffer;

        friend class SessionHandler;

    };

}