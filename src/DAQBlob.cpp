#include <DAQBlob.h>

#include "Packet.h"

#include <stdexcept>

using std::vector;
using std::string;

using namespace DAQCap;


int DataBlob::packetCount() const {

    return packets;

}

vector<uint8_t> DataBlob::data() const {

    return dataBuffer;

}

vector<string> DataBlob::warnings() const {

    return warningsBuffer;

}

vector<Word> DAQCap::packData(const vector<uint8_t> &data) {

    vector<uint64_t> packedData;

    // Without this, the loop will segfault
    if(data.size() < Packet::WORD_SIZE) return packedData;

    packedData.reserve(data.size() / Packet::WORD_SIZE);

    for(
        size_t wordStart = 0; 
        wordStart <= data.size() - Packet::WORD_SIZE; 
        wordStart += Packet::WORD_SIZE
    ) {

        packedData.push_back(0);
        
        for(size_t byte = 0; byte < Packet::WORD_SIZE; ++byte) {

            // NOTE: Without the cast to uint64_t, the shift will be done as if
            //       on a 32-bit integer, causing the first byte to wrap around
            //       and distort the data.
            packedData.back() |= static_cast<uint64_t>(data[wordStart + byte])
                                   << (8 * (Packet::WORD_SIZE - byte - 1));

        }

    }

    return packedData;

}