#include <catch2/catch_test_macros.hpp>

#include <PacketProcessor.h>

#include <numeric>

using std::vector;

using namespace DAQCap;

const int PRELOAD = 14;
const int POSTLOAD = 4;
const int WORD_SIZE = 5;

TEST_CASE("PacketProcessor::process()", "[PacketProcessor]") {

    PacketProcessor processor;
    vector<Packet> packets;

    SECTION("process() returns empty if no packets are provided") {

        DataBlob blob = processor.process(packets);
        packets.clear();

        REQUIRE(blob.packetCount() == 0);
        REQUIRE(blob.data().empty());
        REQUIRE(blob.warnings().empty());

    }

    SECTION("process() succeeds with empty packets") {

        std::vector<uint8_t> empty(PRELOAD + POSTLOAD, 0);

        packets.emplace_back(empty.data(), empty.size());

        DataBlob blob = processor.process(packets);
        packets.clear();

        REQUIRE(blob.packetCount() == 1);
        REQUIRE(blob.data().empty());
        REQUIRE(blob.warnings().empty());

    }

    SECTION("process() succeeds for normal case") {

        // Here we just generate a bunch of unique data and check every byte
        // in the blob

        size_t size = WORD_SIZE * 3;

        vector<uint8_t> data(PRELOAD + POSTLOAD + size, 0);
        vector<uint8_t> data2(PRELOAD + POSTLOAD + size, 0);

        std::iota(data.begin() + PRELOAD, data.end() - POSTLOAD, 0);
        std::iota(data2.begin() + PRELOAD, data2.end() - POSTLOAD, size);

        packets.emplace_back(data.data(), data.size());
        packets.emplace_back(data2.data(), data2.size());

        DataBlob blob = processor.process(packets);
        packets.clear();

        REQUIRE(blob.packetCount() == 2);
        REQUIRE(blob.data().size() == size * blob.packetCount());

        for(int i = 0; i < blob.data().size(); ++i) {

            REQUIRE(blob.data()[i] == i);

        }

    }

    SECTION("process() succeeds with partial words") {

        int FIRST_VAL  = 1;
        int SECOND_VAL = 2;
        int THIRD_VAL  = 3;

        int FIRST_SIZE  = 2 * WORD_SIZE + 1;
        int SECOND_SIZE = 3 * WORD_SIZE;
        int THIRD_SIZE  = WORD_SIZE - 1;

        // First packet will have two words plus an extra byte
        vector<uint8_t> data(
            PRELOAD + POSTLOAD + FIRST_SIZE, 
            FIRST_VAL
        );

        // Second packet will have three words
        vector<uint8_t> data2(
            PRELOAD + POSTLOAD + SECOND_SIZE, 
            SECOND_VAL
        );

        packets.emplace_back(data.data(), data.size());
        packets.emplace_back(data2.data(), data2.size());

        DataBlob blob = processor.process(packets);
        packets.clear();

        REQUIRE(blob.packetCount() == 2);
        
        // The partial word in the first packet should be completed by the
        // second packet, leaving a partial at the end, which should be
        // discarded.
        REQUIRE(
            blob.data().size() 
            == 
            FIRST_SIZE + SECOND_SIZE - (FIRST_SIZE + SECOND_SIZE) % WORD_SIZE
        );

        // Starts in the right place
        REQUIRE(blob.data()[0] == FIRST_VAL);

        // Left side of packet boundary is correct
        REQUIRE(blob.data()[FIRST_SIZE - 1] == FIRST_VAL);

        // Right side of packet boundary is correct
        REQUIRE(blob.data()[FIRST_SIZE] == SECOND_VAL);

        // Now let's make sure partial words across process calls are handled
        // properly.
        // We'll make a packet that finishes the word from the last packet
        vector<uint8_t> data3(
            PRELOAD + POSTLOAD + THIRD_SIZE, 
            THIRD_VAL
        );

        packets.emplace_back(data3.data(), data3.size());

        blob = processor.process(packets);
        packets.clear();

        // Check the packet count
        REQUIRE(blob.packetCount() == 1);

        // The first byte should be from the last packet
        REQUIRE(blob.data()[0] == SECOND_VAL);

        // And the byte after should be from the new packet
        REQUIRE(blob.data()[1] == THIRD_VAL);

    }

    SECTION("fetchData() reports missing packets correctly") {

        size_t packetSize = WORD_SIZE;
        size_t size = PRELOAD + POSTLOAD + packetSize;

        // Shadow the old one bc we need to reset the processor every time
        PacketProcessor processor;
        packets.clear();

        vector<uint8_t> data (size, 0);
        vector<uint8_t> data2(size, 0);
        vector<uint8_t> data3(size, 0);

        SECTION("fetchData() missing packets do not affect received packets") {

            data [size - 1] = 1;
            data2[size - 1] = 2;
            data3[size - 1] = 5;

            packets.emplace_back(data .data(), data .size());
            packets.emplace_back(data2.data(), data2.size());
            packets.emplace_back(data3.data(), data3.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.packetCount() == 3);
            REQUIRE(blob.data().size() == packetSize * 3);

        }

        SECTION("fetchData() reports missing packets within a fetch call") {

            data [size - 1] = 1;
            data2[size - 1] = 2;
            data3[size - 1] = 5;

            packets.emplace_back(data .data(), data .size());
            packets.emplace_back(data2.data(), data2.size());
            packets.emplace_back(data3.data(), data3.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.warnings().size() == 1);

            REQUIRE(
                blob.warnings().front() == "2 packets lost! Packet = 5, Last = 2"
            );

        }

        SECTION("fetchData() reports missing packets across fetch calls") {

            data [size - 1] = 1;
            data2[size - 1] = 2;
            data3[size - 1] = 5;

            packets.emplace_back(data .data(), data .size());
            packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.warnings().empty());

            packets.emplace_back(data3.data(), data3.size());

            blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.warnings().size() == 1);

            REQUIRE(
                blob.warnings().front() == "2 packets lost! Packet = 5, Last = 2"
            );

        }

        SECTION("fetchData() reports multiple missing packets") {

            data [size - 1] = 1;
            data2[size - 1] = 3;
            data3[size - 1] = 5;

            packets.emplace_back(data.data(), data.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.warnings().empty());

            packets.emplace_back(data2.data(), data2.size());
            packets.emplace_back(data3.data(), data3.size());

            blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.warnings().size() == 2);

            REQUIRE(
                blob.warnings().front() == "1 packets lost! Packet = 3, Last = 1"
            );

            REQUIRE(
                blob.warnings().back() == "1 packets lost! Packet = 5, Last = 3"
            );

        }

        SECTION("fetchData() reports missing packets for identical packet numbers") {

            data [size - 1] = 1;
            data2[size - 1] = 1;

            packets.emplace_back(data .data(), data .size());
            packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.warnings().size() == 1);

            REQUIRE(
                blob.warnings().front() == "65535 packets lost! Packet = 1, Last = 1"
            );

        }

        SECTION("fetchData() reports missing packets across overflow boundary") {

            data [size - 1] = 3;
            data2[size - 1] = 1;

            packets.emplace_back(data .data(), data .size());
            packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.warnings().size() == 1);

            REQUIRE(
                blob.warnings().front() == "65533 packets lost! Packet = 1, Last = 3"
            );

        }

        SECTION("fetchData() does not report consecutive packets within a fetch call") {

            data [size - 1] = 1;
            data2[size - 1] = 2;

            packets.emplace_back(data .data(), data .size());
            packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.warnings().empty());

        }

        SECTION("fetchData() does not report consecutive packets across fetch calls") {

            data [size - 1] = 1;
            data2[size - 1] = 2;

            packets.emplace_back(data .data(), data .size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            packets.emplace_back(data2.data(), data2.size());

            blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.warnings().empty());

        }

        SECTION("fetchData() does not report consecutive packets across overflow boundary") {

            data [size - 1] = 0xFF;
            data [size - 2] = 0xFF;

            data2[size - 1] = 0;

            packets.emplace_back(data .data(), data .size());
            packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.warnings().empty());

        }

    }

    SECTION("fetchData() idle word removal") {

        PacketProcessor processor;
        packets.clear();

        SECTION("fetchData() removes idle words") {

            size_t packetSize = WORD_SIZE;
            size_t size = PRELOAD + POSTLOAD + packetSize;

            vector<uint8_t> data (size, 0xFF);

            packets.emplace_back(data.data(), data.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.packetCount() == 1);
            REQUIRE(blob.data().size() == 0);        

        }

        SECTION("fetchData() removes idle words across packet boundaries") {

            size_t packetSize = WORD_SIZE;
            size_t size = PRELOAD + POSTLOAD + packetSize;

            vector<uint8_t> data (size - 1, 0xFF);
            vector<uint8_t> data2(size, 0x00);

            data2[0] = 0xFF;

            packets.emplace_back(data .data(), data .size());
            packets.emplace_back(data2.data(), data2.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.packetCount() == 2);
            REQUIRE(blob.data().size() == WORD_SIZE);

        }

        SECTION("fetchData() removes idle words across fetch calls") {

            size_t packetSize = WORD_SIZE;
            size_t size = PRELOAD + POSTLOAD + packetSize;

            vector<uint8_t> data (size - 1, 0xFF);
            vector<uint8_t> data2(size + 1, 0x00);

            data2[PRELOAD] = 0xFF;

            packets.emplace_back(data.data(), data.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.packetCount() == 1);
            REQUIRE(blob.data().size() == 0);

            packets.emplace_back(data2.data(), data2.size());

            blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.packetCount() == 1);
            REQUIRE(blob.data().size() == WORD_SIZE);

            REQUIRE(blob.data()[0] == 0);

        }

        SECTION("fetchData() removes only the idle words") {

            size_t packetSize = WORD_SIZE * 3;
            size_t size = PRELOAD + POSTLOAD + packetSize;

            vector<uint8_t> data (size, 0x00);
            for(
                auto it = data.begin() + PRELOAD + WORD_SIZE; 
                it != data.begin() + PRELOAD + WORD_SIZE * 2;
                ++it
            ) {

                *it = 0xFF;

            }

            packets.emplace_back(data.data(), data.size());

            DataBlob blob = processor.process(packets);
            packets.clear();

            REQUIRE(blob.packetCount() == 1);
            REQUIRE(blob.data().size() == WORD_SIZE * 2);

            for(uint8_t i = 0; i < blob.data().size(); ++i) {

                blob.data()[i] = 0;

            }

        }

    }

}