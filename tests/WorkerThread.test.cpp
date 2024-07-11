#include <catch2/catch_test_macros.hpp>

#include "WorkerThread.h"

TEST_CASE("forcefail", "[fail]") {
    REQUIRE(1 == 2);
}