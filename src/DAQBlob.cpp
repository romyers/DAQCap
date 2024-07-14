#include <DAQBlob.h>

#include "Packet.h"

#include <stdexcept>

using namespace DAQCap;

std::vector<uint8_t> DataBlob::data() {

    return dataBuffer;

}

std::vector<std::string> DataBlob::warnings() {

    return warningsBuffer;

}

std::vector<uint64_t> DataBlob::pack() {

    // The blob should not contain partial words
    if(dataBuffer.size() % Packet::wordSize() != 0) {

        throw std::logic_error(
            "DataBlob must not contain partial words."
        );

    }

    std::vector<uint64_t> packedData;
    packedData.reserve(dataBuffer.size() / Packet::wordSize());
    
    // TODO: Test this
    //         -- Make sure the right bytes are in the right places
    //         -- Make sure we don't cut the last word off the end
    //         -- Look for more efficient ways to do this
    //              -- Only way I can think of is to memcpy then fix
    //                 endianness if there's a mismatch
    //         -- Make sure the endianness is correct -- check old
    //            code to verify this
    //         -- The decoder will need to pack raw data into words
    //            too. It might not be good to bury this in the
    //            DAQCap library
    //              -- We could consider merging the decode and
    //                 capture libraries though, creating a unified,
    //                 but modular library so that user can strip
    //                 out e.g. ethernet capture if they don't need it.

    for(
        size_t wordStart = 0; 
        wordStart < dataBuffer.size(); 
        wordStart += Packet::wordSize()
    ) {

        int word = 0;
        
        for(size_t byte = 0; byte < Packet::wordSize(); ++byte) {

            wordStart |= dataBuffer[wordStart + byte] << (8 * byte);

        }


    }

    return packedData;

}