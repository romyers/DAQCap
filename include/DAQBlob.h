/**
 * @file DAQBlob.h
 *
 * @brief Packages captured data.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <vector>
#include <string>

namespace DAQCap {

	// OPTIMIZATION: If performance matters, we can return DataBlob values
	//               by const reference instead of by value, at the risk of a 
	//               more complex interface.

	/**
	 * @brief An integer type representing a word of miniDAQ data.
	 */
	typedef uint64_t Word;

	/**
	 * @brief Packs data bytes into words. Excludes any trailing partial words.
	 * 
	 * REQUIRES: 
	 *  - The first byte of data is the first byte of a word.
	 *  - Words are stored consecutively in data.
	 *  - Words in data are in big-endian byte order.
	 * 
	 * @note The result of DataBlob::data() is guaranteed to be well-formed
	 * as input data for packedData().
	 * 
	 * @param data The data to be packed. The data is expected to begin at the
	 * start of a word.
	 */
	std::vector<Word> packData(const std::vector<uint8_t> &data);

	/**
	 * @brief Represents a blob of data fetched from a network device.
	 * 
	 * @note DataBlobs contain exactly an integral number of words.
	 */
	class DataBlob final {

	public:

		DataBlob() = default;

		/**
		 * @brief Gets number of packets in the data blob.
		 */
		int packetCount() const;

		/**
		 * @brief Gets data fetched from the network device.
		 */
		std::vector<uint8_t> data() const;

		/**
		 * @brief Gets warnings that were generated during the fetch.
		 */
		std::vector<std::string> warnings() const;

		/**
		 * @brief An iterator used to traverse the blob's data
		 */
		typedef std::vector<uint8_t>::const_iterator const_iterator;

		/**
		 * @brief Returns a const iterator to the beginning of the data.
		 */
		const_iterator cbegin() const;

		/**
		 * @brief Returns a const iterator to the end of the data.
		 */
		const_iterator cend() const;

	private:

		int packets = 0;

		std::vector<uint8_t> dataBuffer;

		std::vector<std::string> warningsBuffer;

		friend class PacketProcessor;

	};

	/**
	 * @brief Stream insertion operator for DataBlob. Writes the miniDAQ data
	 * to the stream with no padding or metadata.
	 */
	std::ostream &operator<<(
		std::ostream &os, 
		const DataBlob &blob
	);

}