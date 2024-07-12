cmake_minimum_required(VERSION 3.22.1)
project(DAQCap 
    VERSION 1.0 
    DESCRIPTION "Data capture library for the miniDAQ system"
    LANGUAGES C CXX
)

OPTION(BUILD_EXAMPLES "Build the Example Program" OFF)
OPTION(BUILD_TESTING "Build the Test Suite" OFF)

include(FetchContent)

# TODO: Examine this regarding imported libraries:
# https://stackoverflow.com/a/49487630

# We only want to do this if we are the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) 

    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED True)

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    find_package(Doxygen)
    if(Doxygen_FOUND)
        add_subdirectory(docs)
    else()
        message(STATUS "Doxygen not found, not building documentation")
    endif()

    if(BUILD_TESTING)
        include(CTest)
    endif()

endif()

# TODO: Do different things if we're on macOS or Windows?

# Point CMAKE at the project directory for modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules/")

# Find libpcap
find_package(PCAP REQUIRED)

# If libpcap is not found, fetch it from GitHub
if(NOT PCAP_FOUND)

    message(STATUS "libpcap not found, fetching from GitHub")

    FetchContent_Declare(
        libpcap
        GIT_REPOSITORY https://github.com/the-tcpdump-group/libpcap.git
        GIT_TAG libpcap-1.10.4
    )
    FetchContent_MakeAvailable(libpcap)

endif()

# TODO: I want a clean way to swap out network interface implementations. So
#       I'd like the DAQCap library not to include the pcap version, and link
#       it to whatever implementation is relevant. So link to one
#       implementation when compiling the tests, and another when compiling
#       the examples or exposing the library to clients. I don't want clients
#       to have to explicitly link the implementations.

# TODO: Make sure that both ways of including libpcap get linked to correctly

# Build the library
add_library(
    DAQCap 
    STATIC
    src/DAQCap.cpp 
    src/Packet.cpp
    src/NetworkInterface_PCap.cpp
)
target_include_directories(DAQCap PUBLIC include)
target_link_libraries(DAQCap PRIVATE ${PCAP_LIBRARY})

# Build examples
if(BUILD_EXAMPLES)

    add_subdirectory(examples)

endif()

# Build the test suite
# TODO: Only do this if we're the top level project?
if(BUILD_TESTING)

    add_subdirectory(tests)

endif()