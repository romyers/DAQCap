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

    // TODO: I think we will table vv for now. Save it for a time when this
    //       needs to support other data formats
    // TODO: Make Packet an abstract base class and use factory pattern with
    //       data format
    //         -- problem -- how do we override the static methods?
    //              -- How about we say something like 
    //                 Packet::wordSize(std::string format) and use it like
    //                 Packet::wordSize(packet.getFormat())?
    //         -- Another problem -- if packets need the data format to be
    //            created, then the network interface needs to know the data
    //            format. We don't want that.
    //              -- Maybe use an intermediate struct full of raw data that
    //                 we can turn into packets

    /**
     * @brief Represents a raw data packet. Abstracts data format details.
     */
    class Packet {

    public:

        /**
         * @brief Constructs a null packet with no data.
         */
        Packet();

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
         * @brief Checks if this packet is a null packet.
         * 
         * @return True if this packet is null, false otherwise.
         */
        bool isNull() const;

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
         * @brief Gets the size of data words contained in the packet, in
         * bytes.
         * 
         * @return The size of data words contained in the packet, in bytes.
         */
        static size_t wordSize();

        /**
         * @brief Returns the signature of an idle data word. If empty, then
         * idle words do not exist.
         * 
         * @return The signature of an idle data word, or empty if there are no
         * idle words.
         */
        static std::vector<unsigned char> idleWord();

    private:

        // TODO: This can go away once we make this an abstract base class
        std::vector<unsigned char> data;

        unsigned long ID;

    };

} // namespace DAQCap