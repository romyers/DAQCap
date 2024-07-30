/**
 * @file PacketProcessor.h
 *
 * @brief Provides processing functionality for raw network data packets.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <DAQBlob.h>

#include "Packet.h"

#include <vector>
#include <memory>

namespace DAQCap {

	// TODO: Bundle this class in an existing .h?
	// TODO: Reflect semantically that it's more of a packet 'accumulator'

	class PacketProcessor {

	public:

		PacketProcessor();

		// TODO: What if we take an iterator range of packets instead of a
		//       vector?
		/**
		 * @brief Unpacks a vector of packets into a blob, removing idle
		 * words and conducting missing packet checks.
		 * 
		 * @param packets The packets to blobify.
		 * 
		 * @return A blob containing the processed data.
		 */
		DataBlob blobify(const std::vector<Packet> &packet);

		/**
		 * @brief Resets the packet processor.
		 */
		void reset();

	private:

		// The last packet processed
		std::unique_ptr<class Packet> lastPacket;

		// Buffer for unfinished data words at the end of a packet
		std::vector<uint8_t> unfinishedWords;

		/**
		 * @brief Unpacks a vector of packets into a data blob.
		 * 
		 * @param[in] packets The packets to unpack.
		 * @param[out] blob The blob to unpack the packets into.
		 */
		void unpack(
			const std::vector<Packet> &packets, 
			DataBlob &blob
		);

		/**
		 * @brief Scans a vector of packets for warnings and stores the
		 * warnings in a data blob.
		 * 
		 * @param[in] packets The packets to scan for warnings.
		 * @param[out] blob The blob to store the warnings in.
		 */
		void getWarnings(
			const std::vector<Packet> &packets, 
			DataBlob &blob
		);

		/**
		 * @brief Removes idle words from a data blob.
		 * 
		 * @param[in,out] blob The blob to remove idle words from.
		 */
		void removeIdleWords(DataBlob &blob);

	};

}