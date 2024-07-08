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
add_library(DAQCap include/DAQCap.h src/DAQCap.cpp)
target_include_directories(DAQCap PUBLIC include)
target_link_libraries(DAQCap PUBLIC ${PCAP_LIBRARY})

# Build examples
OPTION(BUILD_EXAMPLES "Build the Example Program" OFF)
if(BUILD_EXAMPLES)

    add_executable(p2ecap examples/p2ecap_standalone.cpp)
    target_link_libraries(p2ecap PRIVATE DAQCap)

endif()

# Build the test suite
# TODO: Only do this if we're the top level project?
OPTION(BUILD_TESTING "Build the Test Suite" OFF)
if(BUILD_TESTING)

    # TODO: Find Catch2 if it exists, and only FetchContent if it doesn't

    include(CTest)

    # Get Catch2
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.6.0
    )
    FetchContent_MakeAvailable(Catch2)

    # Build the test executables
    add_executable(testDAQCap src/DAQCap.test.cpp)
    target_link_libraries(testDAQCap PRIVATE Catch2::Catch2WithMain DAQCap)
    add_test(NAME testDAQCap COMMAND testDAQCap)

    # Register the tests
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(testDAQCap)

endif()