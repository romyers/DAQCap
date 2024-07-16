#include "Packet.h"

#include <algorithm>
#include <stdexcept>

using std::string;
using std::vector;

using namespace DAQCap;

/*
 * Packet format:
 *   14 bytes pre-load
 *   N bytes of data
 *   4 bytes post-load (last two bytes are the packet number)
 */

const int    PACKET_NUMBER_OVERFLOW = 65536;
const size_t PRELOAD_BYTES          = 14   ;
const size_t POSTLOAD_BYTES         = 4    ;

const size_t Packet::WORD_SIZE = 5;

const vector<uint8_t> Packet::IDLE_WORD 
    = vector<uint8_t>(Packet::WORD_SIZE, 0xFF);
        
Packet::Packet(const uint8_t *raw_data, size_t size) : packetNumber(0) {

    // NOTE: This relates to the data format from the miniDAQ, not to the 
    //       network interface we're using to get the data.
    //       This logic should not be in the network interface.
    //       The point is that the network interface will give Packet
    //       the raw packet data, and Packet will take that data and extract
    //       the packet size and data section from it.

    if(size < PRELOAD_BYTES + POSTLOAD_BYTES) {

        throw std::invalid_argument(
            string("Packet::Packet: raw_data must be at least ")
                + std::to_string(PRELOAD_BYTES + POSTLOAD_BYTES)
                + " bytes long."
        );

    }

    for(int i = size - 2; i < size; ++i) {

        packetNumber <<= 8;
        packetNumber += raw_data[i];

    }

    data.insert(
        data.end(), 
        raw_data + PRELOAD_BYTES, 
        raw_data + size - POSTLOAD_BYTES
    );

    static unsigned long counter = 0;

    ID = counter;
    ++counter;

}

int Packet::getPacketNumber() const {

    return packetNumber;

}

Packet::const_iterator Packet::cbegin() const { 

    return data.cbegin(); 

}

Packet::const_iterator Packet::cend() const { 

    return data.cend(); 

}

size_t Packet::size() const { 

    // NOTE: Using iterators allows size() to be implementation-independent

    return cend() - cbegin();

}

uint8_t Packet::operator[](size_t index) const {

    if(index >= size()) {

        throw std::out_of_range(
            "Packet::operator[]: index out of range."
        );

    }

    return *(cbegin() + index);

}

int Packet::packetsBetween(const Packet &first, const Packet &second) {

    // This makes sure that first is always the older packet
    if(first.ID > second.ID) {

        return packetsBetween(second, first);

    }

    int diff = 0;
    
    if(first.getPacketNumber() > second.getPacketNumber()) {

        diff = (second.getPacketNumber() + PACKET_NUMBER_OVERFLOW)
                - (first.getPacketNumber() + 1);

    } else {

        diff = second.getPacketNumber() - (first.getPacketNumber() + 1);

    }
    
    if(diff < 0) diff += PACKET_NUMBER_OVERFLOW;

    return diff;

}