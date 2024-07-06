#include <catch2/catch_test_macros.hpp>

#include <DAQCap.h>

TEST_CASE("forcefail", "[temp]") {
    REQUIRE(1 == 2);
}