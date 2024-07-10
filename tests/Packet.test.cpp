#include <catch2/catch_test_macros.hpp>

#include <Packet.h>

TEST_CASE("forcefail", "[fail]") {
    REQUIRE(1 == 2);
}