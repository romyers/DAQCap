# TODO: Find Catch2 if it exists, and only FetchContent if it doesn't

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

# Build the mock version of the DAQCap library
# TODO: Cleaner way to do this? Can we avoid building two versions of the
#       library? Try having DAQCap use an interface library that hits
#       different versions of NetworkInterface
add_library(
    DAQCapMock 
    ${SRC_DIR}/DAQCap.cpp 
    ${SRC_DIR}/Packet.cpp
    NetworkInterface.mock.cpp
)
target_include_directories(DAQCapMock PUBLIC ${INCLUDE_DIR})
target_include_directories(DAQCapMock PRIVATE ${SRC_DIR})

# Build the test executables
add_executable(testDAQCap DAQCap.test.cpp)
target_link_libraries(testDAQCap PRIVATE Catch2::Catch2WithMain DAQCapMock)
add_test(NAME testDAQCap COMMAND testDAQCap)
catch_discover_tests(testDAQCap)

add_executable(testPacket Packet.test.cpp ${SRC_DIR}/Packet.cpp)
target_link_libraries(testPacket PRIVATE Catch2::Catch2WithMain)
target_include_directories(testPacket PRIVATE ${SRC_DIR})
add_test(NAME testPacket COMMAND testPacket)
catch_discover_tests(testPacket)