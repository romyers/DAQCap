cmake_minimum_required(VERSION 3.22.1)
project(DAQCap 
    VERSION 1.0 
    DESCRIPTION "Data capture library for the miniDAQ system"
    LANGUAGES C CXX
)
include(FetchContent)

# TODO: We need to do setcap stuff. See:
# https://stackoverflow.com/questions/41254902/cmake-run-script-for-install-target
# https://stackoverflow.com/questions/31540189/is-it-possible-to-execute-a-command-as-a-super-user-while-using-cmake
# https://stackoverflow.com/questions/72012474/integrate-granting-of-capabilities-into-the-build-process

# We only want to do this if we are the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) 

    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED True)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    find_package(Doxygen)
    if(Doxygen_FOUND)
        add_subdirectory(docs)
    else()
        message(STATUS "Doxygen not found, not building documentation")
    endif()

endif()

# TODO: Do different things if we're on macOS or Windows?

# Point CMAKE at the project directory for modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/modules/")

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

# Build the library
add_library(
    DAQCap 
    src/DAQCap.cpp 
    src/NetworkInterface_PCap.cpp
    src/Packet.cpp
)
target_include_directories(DAQCap PUBLIC include)
target_link_libraries(DAQCap PRIVATE ${PCAP_LIBRARY})

# Build examples
OPTION(BUILD_EXAMPLES "Build the Example Program" OFF)
if(BUILD_EXAMPLES)

    add_subdirectory(examples)

endif()

# Build the test suite
# TODO: Only do this if we're the top level project?
OPTION(BUILD_TESTING "Build the Test Suite" OFF)
if(BUILD_TESTING)

    add_subdirectory(tests)

endif()