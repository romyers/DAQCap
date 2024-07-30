cmake_minimum_required(VERSION 3.22.1)
project(DAQCap 
	VERSION 1.0 
	DESCRIPTION "Data capture library for the miniDAQ system"
	LANGUAGES C CXX
)

OPTION(BUILD_EXE "Build the executable ecap program" OFF)
OPTION(BUILD_TESTING "Build the Test Suite" OFF)
OPTION(
	INCLUDE_NETWORK_LIBRARY 
	"Include a network interface library if one is found" 
	ON
)

include(FetchContent)

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

	# Build tests
	if(BUILD_TESTING)
	
		include(CTest)
		add_subdirectory(tests)

	endif()

	# Build examples
	if(BUILD_EXE)

		add_subdirectory(examples)

	endif()

endif()

# Point CMake at the project directory for modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules/")

# Find libpcap, and fetch it if it's not found
find_package(PCAP REQUIRED)
if(NOT PCAP_FOUND)

	message(STATUS "libpcap not found, fetching from GitHub")

	FetchContent_Declare(
		libpcap
		GIT_REPOSITORY https://github.com/the-tcpdump-group/libpcap.git
		GIT_TAG libpcap-1.10.4
	)
	FetchContent_MakeAvailable(libpcap)

	set(PCAP_FOUND TRUE)

endif()

# TODO: Look at this for debug logging:
#       https://www.reddit.com/r/cpp_questions/comments/obndlq/help_use_cmake_to_define_and_test_preprocessor/

# Build the library
add_library(
	DAQCap 
	SHARED
	src/DAQCap.cpp 
	src/DAQBlob.cpp
	src/Packet.cpp
	src/PacketProcessor.cpp
)
target_link_libraries(DAQCap PRIVATE ${PCAP_LIBRARY})
target_include_directories(DAQCap PUBLIC include)