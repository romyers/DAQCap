#include <catch2/catch_test_macros.hpp>

#include <DAQBlob.h>

#include <thread>
#include <numeric>
#include <sstream>

using std::vector;

const size_t WORD_SIZE = 5;

using namespace DAQCap;
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("DAQCap::packData()") {

    SECTION("packData() returns empty for empty input") {

        vector<uint8_t> empty;

        vector<Word> packedData = packData(empty);

        REQUIRE(packedData.empty());

    }

    SECTION("packData() returns empty for input smaller than wordsize") {

        vector<uint8_t> small(WORD_SIZE - 1);

        vector<Word> packedData = packData(small);

        REQUIRE(packedData.empty());

    }

    SECTION("packData() is correct for input of exactly wordsize") {

        vector<uint8_t> data(WORD_SIZE);

        std::iota(data.begin(), data.end(), 0);

        vector<Word> packedData = packData(data);

        REQUIRE(packedData.size() == 1);
        REQUIRE(packedData[0] == 0x0001020304);

    }

    SECTION("packData() is correct for one word plus a partial word") {

        vector<uint8_t> data(WORD_SIZE + 1);

        std::iota(data.begin(), data.end(), 0);

        vector<Word> packedData = packData(data);

        REQUIRE(packedData.size() == 1);
        REQUIRE(packedData[0] == 0x0001020304);

    }

    SECTION("packData() is correct for multiple words") {

        vector<uint8_t> data(WORD_SIZE * 3);

        std::iota(data.begin(), data.end(), 0);

        vector<Word> packedData = packData(data);

        REQUIRE(packedData.size() == 3);
        REQUIRE(packedData[0] == 0x0001020304);
        REQUIRE(packedData[1] == 0x0506070809);
        REQUIRE(packedData[2] == 0x0a0b0c0d0e);

    }

    SECTION("packData() is correct for multiple words plus a partial") {

        vector<uint8_t> data(WORD_SIZE * 4 - 1);

        std::iota(data.begin(), data.end(), 0);

        vector<Word> packedData = packData(data);

        REQUIRE(packedData.size() == 3);
        REQUIRE(packedData[0] == 0x0001020304);
        REQUIRE(packedData[1] == 0x0506070809);
        REQUIRE(packedData[2] == 0x0a0b0c0d0e);

    }

}