/**
 * @file Packet.h
 *
 * @brief Represents a raw network data packet.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <cstddef>
#include <vector>

namespace DAQCap {

    // TODO: Packet will hide data format details

    // TODO: Decide where to do idle word checking.
    //         -- I'd like everything to do with the miniDAQ data format to 
    //            be in one place. That's packets and signals. Word size
    //            belongs with signals. But we need to exclude idle words
    //            from DAT files.

    /**
     * @brief Represents a raw data packet. Abstracts data format details.
     */
    class Packet {

    public:

        /**
         * @brief Constructor.
         * 
         * @param raw_data A pointer to the raw packet data, stored as an array
         * of unsigned chars. Packet does not need the data pointer to persist
         * after construction.
         * 
         * @param size The size of the raw packet data array.
         * 
         * @throws std::invalid_argument If size is too small to represent a
         * packet.
         */
        Packet(const unsigned char *raw_data, size_t size);

        /**
         * @brief Returns the packet number associated with this packet.
         */
        int getPacketNumber() const;

        /**
         * @brief Returns the size of the data portion of the packet.
         */
        size_t size() const;

        /**
         * @brief An iterator used to traverse the data portion of the packet.
         */
        typedef std::vector<unsigned char>::const_iterator const_iterator;

        /**
         * @brief Returns a const iterator to the beginning of the data portion
         * of the packet.
         */
        const_iterator cbegin() const;

        /**
         * @brief Returns a const iterator to the end of the data portion of
         * the packet.
         */
        const_iterator cend() const;

        /**
         * @brief Returns the byte at the given index in the data portion of
         * the packet.
         * 
         * @param index The index of the byte to return.
         * 
         * @return The byte at the given index.
         * 
         * @throws std::out_of_range If index is greater than or equal to the
         * size of the data portion of the packet.
         */
        unsigned char operator[](size_t index) const;

        /**
         * @brief Returns the number of packets that should exist between
         * two packets.
         * 
         * @note This function is symmetric. That is, for two packets
         * a and b, packetsBetween(a, b) == packetsBetween(b, a).
         * 
         * @param first A packet.
         * @param second A packet.
         * 
         * @return The number of packets that should exist between the two
         * packets.
         */
        static int packetsBetween(const Packet &first, const Packet &second);

        /**
         * @brief The size of data words contained in the packet, in bytes
         */
        static size_t WORD_SIZE;

        /**
         * @brief The signature of an idle data word.
         */
        static std::vector<unsigned char> IDLE_WORD;

    private:

        std::vector<unsigned char> data;

        unsigned long ID;

    };

} // namespace DAQCap