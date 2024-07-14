#include "Packet.h"

#include <algorithm>
#include <stdexcept>

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

size_t Packet::wordSize() {

    return 5;

}

std::vector<uint8_t> Packet::idleWord() {

    return std::vector<uint8_t>(Packet::wordSize(), 0xFF);

}

Packet::Packet() : packetNumber(0) {

    // A 0 ID denotes a null packet
    ID = 0;

}
        
Packet::Packet(const uint8_t *raw_data, size_t size) : packetNumber(0) {

    if(size < PRELOAD_BYTES + POSTLOAD_BYTES) {

        throw std::invalid_argument(
            std::string("Packet::Packet: raw_data must be at least ")
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

    // A 0 ID denotes a null packet, so real packets should start at 1
    static unsigned long counter = 1;

    ID = counter;
    ++counter;

}


Packet::operator bool() const {

    return ID != 0;

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

    // This makes sure that first is always the packet that came first.
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