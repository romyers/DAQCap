\mainpage
\tableofcontents

# DAQCap -- ATLAS MiniDAQ Data Capture Library

This library provides tools for capturing data from miniDAQ MDT and
sMDT chambers via an ethernet connection, together with an optional executable
program that captures miniDAQ data and writes it to a DAT file.

## Setup

This library was developed for Linux, and requires C++11 support or better.
Windows and MacOS environments have not been tested.

DAQCap is based on
[libpcap](https://github.com/the-tcpdump-group/libpcap/tree/master).
If libpcap is not present, it will be installed during the build process.
Installing libpcap requires:
  - [Flex](https://github.com/westes/flex) version 2.5.31 or later.
  - One of:
      - [Bison](https://ftp.gnu.org/gnu/bison/)
      - [Berkely YACC](https://ftp.gnu.org/gnu/bison/)
    
    or another compatible version of YACC.

Once either libpcap or its prerequisites are included, download and build the 
DAQCap library either 
[manually](#manual-installation),
with the included [install script](#build-with-installer),
or with [CMake FetchContent](#install-with-fetchcontent).

### Manual Installation

Clone the repository and build with CMake:
```console
$ cmake [PATH_TO_SOURCE_DIRECTORY]
$ make
```
By default, the library will not be built with tests or the optional
executable. To build with tests or the optional executable, run
CMake with the `BUILD_TESTING` or `BUILD_EXE` options set to `ON`:
```console
$ cmake -DBUILD_TESTING=ON -DBUILD_EXE=ON [PATH_TO_SOURCE_DIRECTORY]
```

### Build with Installer

Clone the repository, navigate to the project directory, 
run:
```console
$ source install.sh
```
and provide root privileges when prompted.

This will build the library together with the optional executable and install
them in a subdirectory of the project directory called 'build'.

### Install with FetchContent

You can easily add this library to a CMake project using the `FetchContent`
module:
```CMake
include(FetchContent)
FetchContent_Declare(
    DAQCap
    GIT_REPOSITORY https://github.com/romyers/DAQCap
    GIT_TAG v1.0 # Or later version
)
FetchContent_MakeAvailable(DAQCap)
```
Then link the library to your target, e.g:
```CMake
target_link_libraries(my_target PRIVATE DAQCap)
```
And include the header in your source files:
```cpp
#include <DAQCap.h>
```

## Usage

To use this library, link it to your executable and include the `DAQCap.h` 
header in your source files.

Any executable using this library on Linux must have CAP_NET_RAW file 
capabilities:
```console
$ sudo setcap cap_net_raw=eip [EXECUTABLE]
```
This includes the optional executable included with DAQCap. If built
using the install script, the optional executable will be given this
capability automatically.

A full [example program](#example-reading-data) is included later in this
document. See the [documentation section](#documentation) for further
information.

## Documentation

The DAQCap library features full API documentation via Doxygen. To build the
documentation, navigate to the CMake build directory and run:
```console
$ make docs
```
This will generate Doxygen documentation for the public API in the docs/doxygen
folder under the project directory.

## Example: Reading Data

The following is a simple program illustrating the use of the DAQCap
library:
```cpp
#include <DAQCap.h>

#include <vector>
#include <memory>
#include <chrono>
#include <iostream>

int main() {

    // The first step is to create a session handler. Library functionality
    // will be accessed through this handler.
    DAQCap::SessionHandler handler;

    // Next we'll get a list of available network devices.
    std::vector<std::shared_ptr<DAQCap::Device>> devices;
    try {

        // This will throw an exception if the device list could not be
        // obtained.
        devices = handler.getAllNetworkDevices();

    } catch(...) {

        return 1;

    }

    // Pick a device. We could prompt the user to select a device, or we could
    // accept a device name and use handler.getNetworkDevice([device name])
    // to look for and retrieve the associated device.
    std::shared_ptr<DAQCap::Device> d = devices[0];

    // Next, we'll start a capture session on the selected device.
    try {

        // This too will throw an exception if something goes wrong.
        handler.startSession(d);

    } catch(...) {

        return 1;

    }

    // We're now ready to read the data. Loop until we've read some number
    // of packets. Here, we'll store the packet data in a buffer, but we could
    // e.g. write to a file or feed the data to a decoding library instead.
    int packets = 0;
    std::vector<uint8_t> dataBuffer;
    while(packets < 10000) {

        // This will block until packet data is received, then return
        // the data as a DAQData::DataBlob.
        DAQCap::DataBlob data;
        try {
            data = handler.fetchData();
        } catch(...) { continue; }

        // Another version of this call, this time specifying a timeout
        // and a maximum number of packets to read. This will try to read
        // at most 50 packets, and throw a DAQCap::timeout_exception
        // if it fails to complete the read within 10 seconds.
        /*
        try {

            DAQCap::DataBlob data = handler.fetchData(
                std::chrono::seconds(10),
                50
            );

        } catch(const DAQCap::timeout_exception &e) {
        
            continue;

        } catch(...) { continue; }
        */

        // Let's report any issues that occurred during the read.
        for(std::string warning : data.warnings()) {

                std::cerr << warning << std::endl;

        }

        // Now we'll buffer the data using DataBlob's iterators
        dataBuffer.insert(
            dataBuffer.end(),
            data.cbegin(),
            data.cend()
        );
        
        // And record the packet count
        packets += data.packetCount();

    }

    return 0;

}
```

## For Developers

Documentation for the internal DAQCap API is available. To generate it, run:
```console
$ cmake -DBUILD_INTERNAL_DOCS=ON [PATH_TO_SOURCE_DIRECTORY]
$ make docs
```

For more information, see the [developer notes](Developer_Notes.md).