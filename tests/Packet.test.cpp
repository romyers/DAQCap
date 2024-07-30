#include <catch2/catch_test_macros.hpp>

#include <Packet.h>

#include <numeric>

using std::vector;

using namespace DAQCap;

const int    PACKET_NUMBER_OVERFLOW = 65535;
const size_t PRELOAD                = 14   ;
const size_t POSTLOAD               = 4    ;
const size_t WORD_SIZE              = 5    ;

TEST_CASE("Packet constructor", "[Packet]") {

	SECTION("Packet constructor throws an exception if the size is too small") {

		uint8_t data[PRELOAD + POSTLOAD - 1];

		REQUIRE_THROWS_AS(Packet(data, PRELOAD + POSTLOAD - 1), std::invalid_argument);

	}

	SECTION("Packet constructor does not throw an exception if the size is sufficiently large") {

		uint8_t data[PRELOAD + POSTLOAD];

		REQUIRE_NOTHROW(Packet(data, PRELOAD + POSTLOAD));

	}

}   

TEST_CASE("Packet::size()", "[Packet]") {

	SECTION("Packet::size() returns 0 for an empty packet") {

		size_t packetSize = 0;
		size_t size = PRELOAD + POSTLOAD + packetSize;

		uint8_t data[size];
		Packet packet(data, size);

		REQUIRE(packet.size() == packetSize);

	}

	SECTION("Packet::size() returns the size of the data portion of the packet") {

		size_t packetSize = 10;
		size_t size = PRELOAD + POSTLOAD + packetSize;

		uint8_t data[size];
		Packet packet(data, size);

		REQUIRE(packet.size() == packetSize);

	}
	

}

TEST_CASE("Packet::getPacketNumber()", "[Packet]") {

	size_t packetSize = 10;
	size_t size = PRELOAD + POSTLOAD + packetSize;

	vector<uint8_t> data(size, 0);

	SECTION("Packet number does not rely on all but last two postload bytes") {

		for(int i = size - POSTLOAD; i < size - 2; ++i) {
			data[i] = 0x01;
		}
		Packet packet(data.data(), size);

		REQUIRE(packet.getPacketNumber() == 0);    

	}

	SECTION("Packet number is extracted correctly") {

		data[size - 2] = 0x01;
		data[size - 1] = 0x02;
		Packet packet(data.data(), size);

		REQUIRE(packet.getPacketNumber() == 0x0102);

	}

}

TEST_CASE("Packet::packetsBetween()", "[Packet]") {

	int packetSize = 0;
	int size = PRELOAD + POSTLOAD + packetSize;

	vector<uint8_t> data(size, 0);
	vector<uint8_t> data2(size, 0);

	SECTION("packetsBetween() is zero between consecutive packets") {

		data[size - 2] = 0x01;
		data[size - 1] = 0x02;

		data2[size - 2] = 0x01;
		data2[size - 1] = 0x03;

		Packet packet(data.data(), size);
		Packet packet2(data2.data(), size);

		REQUIRE(Packet::packetsBetween(packet, packet2) == 0);

	}

	SECTION("packetsBetween() is maximal between packets with the same packet number") {

		data[size - 2] = 0x01;
		data[size - 1] = 0x02;

		data2[size - 2] = 0x01;
		data2[size - 1] = 0x02;

		Packet packet(data.data(), size);
		Packet packet2(data2.data(), size);

		REQUIRE(Packet::packetsBetween(packet, packet2) == 0xFFFF);

	}

	SECTION("packetsBetween() is symmetric") {

		data[size - 2] = 0x92;
		data[size - 1] = 0x3A;

		data2[size - 2] = 0x46;
		data2[size - 1] = 0xF3;

		Packet packet(data.data(), size);
		Packet packet2(data2.data(), size);
		
		REQUIRE(
			Packet::packetsBetween(packet, packet2) == 
			Packet::packetsBetween(packet2, packet)
		);

	}

	SECTION("packetsBetween() is correct across the overflow boundary") {

		data[size - 2] = 0xFF;
		data[size - 1] = 0xFF;

		data2[size - 2] = 0x00;
		data2[size - 1] = 0x00;

		Packet packet(data.data(), size);
		Packet packet2(data2.data(), size);

		REQUIRE(Packet::packetsBetween(packet, packet2) == 0);

	}

	SECTION("packetsBetween() is correct for extrema not crossing overflow boundary") {

		data[size - 2] = 0x00;
		data[size - 1] = 0x00;

		data2[size - 2] = 0xFF;
		data2[size - 1] = 0xFF;

		Packet packet(data.data(), size);
		Packet packet2(data2.data(), size);

		REQUIRE(Packet::packetsBetween(packet, packet2) == 0xFFFE);

	}

	SECTION("packetsBetween() is correct for smaller first packet") {

		data[size - 2] = 0x12;
		data[size - 1] = 0x53;

		data2[size - 2] = 0x55;
		data2[size - 1] = 0x64;

		Packet packet(data.data(), size);
		Packet packet2(data2.data(), size);

		REQUIRE(Packet::packetsBetween(packet, packet2) == 0x4310);

	}

	SECTION("packetsBetween() is correct for smaller second packet") {

		data[size - 2] = 0x55;
		data[size - 1] = 0x64;

		data2[size - 2] = 0x12;
		data2[size - 1] = 0x53;

		Packet packet(data.data(), size);
		Packet packet2(data2.data(), size);

		REQUIRE(Packet::packetsBetween(packet, packet2) == 0xBCEE);

	}

}

TEST_CASE("Packet::const_iterator", "[Packet]") {

	size_t packetSize = 10;
	size_t size = PRELOAD + POSTLOAD + packetSize;

	vector<uint8_t> data(size, 0);

	std::iota(data.begin(), data.end(), 0);

	Packet packet(data.data(), size);

	SECTION("Packet::cbegin() refers to the correct byte") {

		REQUIRE(*packet.cbegin() == data[PRELOAD]);

	}

	SECTION("Packet::cend() refers to the correct byte") {

		REQUIRE(*(packet.cend() - 1) == data[size - POSTLOAD - 1]);

	}

	SECTION("Iterating over Packet gives correct values") {

		for(auto it = packet.cbegin(); it != packet.cend(); ++it) {
			REQUIRE(*it == data[(it - packet.cbegin()) + PRELOAD]);
		}

	}

}

TEST_CASE("Packet::operator[]", "[Packet]") {

	size_t packetSize = 10;
	size_t size = PRELOAD + POSTLOAD + packetSize;

	vector<uint8_t> data(size, 0);

	std::iota(data.begin(), data.end(), 0);

	Packet packet(data.data(), size);
	
	SECTION("Packet::operator[] throws if the index is out of range") {

		REQUIRE_THROWS_AS(packet[packetSize], std::out_of_range);

	}

	SECTION("Packet::operator[] returns the correct byte") {

		for(int i = 0; i < packetSize; ++i) {
			REQUIRE(packet[i] == data[i + PRELOAD]);
		}

	}

}

TEST_CASE("Packet::WORD_SIZE is correctly set", "[Packet]") {

	REQUIRE(Packet::WORD_SIZE == WORD_SIZE);

}

TEST_CASE("Packet::IDLE_WORD is correctly set", "[Packet]") {

	vector<uint8_t> idleWord = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	REQUIRE(Packet::IDLE_WORD == idleWord);

}