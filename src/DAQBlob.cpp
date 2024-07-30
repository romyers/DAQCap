#include <DAQBlob.h>

#include "Packet.h"

#include <stdexcept>
#include <iostream>

using std::vector;
using std::string;
using std::ostream;

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
	packedData.reserve(data.size() / Packet::WORD_SIZE);

	// NOTE: Since data.size() and Packet::WORD_SIZE are unsigned, we need to
	//       cast to int to avoid underflow for small data.size() in the loop
	//       comparison.
	for(
		int wordStart = 0; 
		wordStart <= (int)data.size() - (int)Packet::WORD_SIZE; 
		wordStart += Packet::WORD_SIZE
	) {

		// Convert a word of raw bytes into a uint64_t
		packedData.push_back(0);
		for(size_t byte = 0; byte < Packet::WORD_SIZE; ++byte) {

			// memcpy would be faster, but runs into byte order issues.

			// NOTE: Without the cast to uint64_t, the shift will be done as if
			//       on a 32-bit integer, causing the first byte to wrap around
			//       and distort the data.
			packedData.back() |= static_cast<uint64_t>(data[wordStart + byte])
								   << (8 * (Packet::WORD_SIZE - byte - 1));

		}

	}

	return packedData;

}

DataBlob::const_iterator DataBlob::cbegin() const {

	return dataBuffer.cbegin();

}

DataBlob::const_iterator DataBlob::cend() const {

	return dataBuffer.cend();

}

ostream &DAQCap::operator<<(ostream &os, const DataBlob &blob) {

	os.write((char*)blob.data().data(), blob.data().size());

	return os;

}