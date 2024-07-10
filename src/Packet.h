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

namespace DAQCap {

    /**
     * @brief Represents a raw data packet.
     * 
     * NOTE: Packet manages its own memory, and will delete its data array
     * upon destruction.
     */
    struct Packet {

        /**
         * @brief Constructor.
         * 
         * @param data A pointer to the packet data, stored as an array of
         * unsigned chars. Packet does not need the data pointer to persist
         * after construction.
         * 
         * @param size The size of the packet data array.
         */
        Packet(const unsigned char *data, size_t size);
        ~Packet();
        
        Packet           (const Packet  &other)         ;
        Packet           (      Packet &&other) noexcept;
        Packet &operator=(const Packet  &other)         ;
        Packet &operator=(      Packet &&other) noexcept;

        /**
         * @brief A pointer to the packet data, stored as an array of 
         * unsigned chars. The size of the array is given by Packet::size.
         * 
         * NOTE: Packet manages its own memory, so Packet::data is tied
         * to the lifetime of the Packet object.
         */
        unsigned char *data;

        /**
         * @brief The size of the packet data array.
         */
        size_t size;

    };

}