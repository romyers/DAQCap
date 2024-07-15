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
#include <stdint.h>

namespace DAQCap {

    /**
     * @brief Represents a raw data packet. Abstracts data format details.
     */
    class Packet {

    public:

        /**
         * @brief The size in bytes of data words contained in the packet.
         */
        static const size_t WORD_SIZE;

        /**
         * @brief The signature of an idle data word. 
         * 
         * If IDLE_WORD is empty, then idle words do not exist.
         */
        static const std::vector<uint8_t> IDLE_WORD;

        /**
         * @brief Constructs a null packet with no data.
         */
        Packet();

        /**
         * @brief Constructs a packet from the given data.
         * 
         * @param raw_data A pointer to the raw packet data, stored as an array
         * of uint8_ts. Packet does not need the data pointer to persist
         * after construction.
         * 
         * @param size The size of the raw packet data array.
         * 
         * @throws std::invalid_argument If size is too small to represent a
         * packet.
         */
        Packet(const uint8_t *raw_data, size_t size);

        /**
         * @brief Returns the packet number associated with this packet.
         * 
         * @note The null packet will have a packet number of 0.
         */
        int getPacketNumber() const;

        /**
         * @brief Returns the size of the data portion of the packet.
         */
        size_t size() const;

        /**
         * @brief An iterator used to traverse the data portion of the packet.
         */
        typedef std::vector<uint8_t>::const_iterator const_iterator;

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
        uint8_t operator[](size_t index) const;

        /**
         * @brief Returns the number of packets that should exist between
         * two packets. The order in which packets were created is taken
         * into account.
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
         * @brief Converts the packet to a boolean value. Returns true if the
         * packet is not null, and false otherwise.
         */
        explicit operator bool() const;

    private:

        int packetNumber;

        std::vector<uint8_t> data;

        unsigned long ID;

    };

} // namespace DAQCap