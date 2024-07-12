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
        
Packet::Packet(const unsigned char *raw_data, size_t size) {

    if(size < PRELOAD_BYTES + POSTLOAD_BYTES) {

        throw std::invalid_argument(
            std::string("Packet::Packet: raw_data must be at least ")
                + std::to_string(PRELOAD_BYTES + POSTLOAD_BYTES)
                + " bytes long."
        );

    }

    data.insert(data.end(), raw_data, raw_data + size);

    static unsigned long counter = 0;

    ID = counter;
    ++counter;

}

int Packet::getPacketNumber() const {

    int number = 0;

    for(int i = data.size() - 2; i < data.size(); ++i) {

        number <<= 8;
        number += data[i];

    }

    return number;

}

// TODO: Definitely need to test these iterators

Packet::const_iterator Packet::cbegin() const { 
    return data.cbegin() + PRELOAD_BYTES; 
}

Packet::const_iterator Packet::cend() const { 
    return data.cend() - POSTLOAD_BYTES; 
}

int Packet::packetsBetween(const Packet &first, const Packet &second) {

    // This makes sure that first is always the packet that came first.
    if(first.ID > second.ID) {

        return packetsBetween(second, first);

    }

    // TODO: The following code could be made more obvious, or at least
    //       commented

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

size_t Packet::size() const { 

    return cend() - cbegin();

}

unsigned char Packet::operator[](size_t index) const {

    if(index >= size()) {

        throw std::out_of_range(
            "Packet::operator[]: index out of range."
        );

    }

    return *(cbegin() + index);

}