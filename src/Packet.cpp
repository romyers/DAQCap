#include "Packet.h"

#include <algorithm>

using namespace DAQCap;
        
Packet::Packet(const unsigned char *data, size_t size)
    : data(new unsigned char[size]), size(size) {

    // TODO: In practice, this copy is unnecessary. Packet can just assume
    //       ownership of the data pointer. It's useful to copy the data here
    //       only to make Packet more obvious to future developers.
    std::copy(data, data + size, this->data);

}

Packet::~Packet() {

    delete[] data;
    data = nullptr;

}

Packet::Packet(const Packet &other) 
    : data(new unsigned char[other.size]), size(other.size) {

    std::copy(other.data, other.data + size, data);

}
        
Packet::Packet(Packet &&other) noexcept
    : data(other.data), size(other.size) {

    other.data = nullptr;
    other.size = 0;

}

Packet &Packet::operator=(const Packet &other) {

    if(this == &other) return *this;

    delete[] data;
    data = new unsigned char[other.size];
    size = other.size;

    std::copy(other.data, other.data + size, data);

    return *this;

}

Packet &Packet::operator=(Packet &&other) noexcept {

    if(this == &other) return *this;

    delete[] data;
    data = other.data;
    size = other.size;

    other.data = nullptr;
    other.size = 0;

    return *this;

}