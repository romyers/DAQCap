cmake_minimum_required(VERSION 3.22.1)
project(DAQCap 
    VERSION 1.0 
    DESCRIPTION "Data capture library for the miniDAQ system"
    LANGUAGES CXX
)

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

add_library(DAQCap include/DAQCap.h src/DAQCap.cpp)
target_include_directories(DAQCap PUBLIC include)

# OPTION(BUILD_EXAMPLES "Build the Example Program" OFF)
# if(BUILD_EXAMPLES)
# 
#     add_executable(SimpleExample examples/SimpleExample.cpp)
#     target_link_libraries(SimpleExample PRIVATE SimpleProgressBar)
# 
#     add_executable(CustomizationExample examples/CustomizationExample.cpp)
#     target_link_libraries(CustomizationExample PRIVATE SimpleProgressBar)
# 
# endif()

OPTION(BUILD_TESTING "Build the Test Suite" OFF)
if(BUILD_TESTING)

    # TODO: Find Catch2 if it exists, and only FetchContent if it doesn't

    include(FetchContent)
    include(CTest)

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.6.0
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(testDAQCap src/DAQCap.test.cpp)
    target_link_libraries(testDAQCap PRIVATE Catch2::Catch2WithMain DAQCap)
    add_test(NAME testDAQCap COMMAND testDAQCap)

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(testDAQCap)

endif()