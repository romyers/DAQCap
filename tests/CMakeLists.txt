# Get Catch2
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.6.0
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# Build the test executables
add_executable(testDAQCap DAQCap.test.cpp)
target_link_libraries(testDAQCap PRIVATE Catch2::Catch2WithMain DAQCap)
target_include_directories(testDAQCap PRIVATE ${SRC_DIR})
add_test(NAME testDAQCap COMMAND testDAQCap)
catch_discover_tests(testDAQCap)

add_executable(testPacket Packet.test.cpp ${SRC_DIR}/Packet.cpp)
target_link_libraries(testPacket PRIVATE Catch2::Catch2WithMain)
target_include_directories(testPacket PRIVATE ${SRC_DIR})
add_test(NAME testPacket COMMAND testPacket)
catch_discover_tests(testPacket)

add_executable(
    testPacketProcessor 
    PacketProcessor.test.cpp 
    ${SRC_DIR}/PacketProcessor.cpp
)
target_link_libraries(testPacketProcessor PRIVATE Catch2::Catch2WithMain DAQCap)
target_include_directories(testPacketProcessor PRIVATE ${SRC_DIR})
add_test(NAME testPacketProcessor COMMAND testPacketProcessor)
catch_discover_tests(testPacketProcessor)