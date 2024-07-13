/**
 * @file DAQCap.h
 *
 * @brief Main interface for the DAQCap library.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <vector>
#include <stdexcept>
#include <string>
#include <chrono>

#include <DAQCapDevice.h>

// TODO: Documentation, especially throws

// TODO: Idea -- we can use CMake to pass in a flag that indicates whether we
//       have pcap. If we don't, we can use the flag to skip the include and
//       stub the network interface, then flag the library as disabled via
//       the public interface.

namespace DAQCap {

    struct DataBlob;

    // TODO: A better way to do this
    // TODO: With C++17, use std::optional
    const int ALL_PACKETS = -1;
    const std::chrono::milliseconds FOREVER(-1);

    /**
     * @brief Exception thrown to signal that a timeout occurred.
     */
    class timeout_exception : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    /**
     * @brief Manages a session with a network device. 
     * 
     * The SessionHandler class provides a high-level interface for fetching
     * data from a network device. One SessionHandler should correspond to one
     * data capture session on one network device. If you need to capture data
     * from multiple devices, or in multiple sessions, you should create a
     * separate SessionHandler for each device or session.
     * 
     * @note This class is not thread-safe. Do not concurrently fetch packets
     * from two different threads, even if they are using different devices.
     */
    class SessionHandler {

    public:

        /**
         * @brief Constructs a SessionHandler object for the given device.
         * 
         * @throws std::runtime_error if the device does not exist or could not
         * be initialized.
         */
        SessionHandler(const Device &device);
        virtual ~SessionHandler();

        SessionHandler(const SessionHandler &other)            = delete;
        SessionHandler &operator=(const SessionHandler &other) = delete;

        /**
         * @brief Thread-safe method that interrupts calls to fetchData().
         */
        virtual void interrupt();

        /**
         * @brief Waits for and retrieves data from the network device
         * associated with this SessionHandler.
         * 
         * Waits for data to arrive on the network device associated with this
         * SessionHandler, then returns a DataBlob containing the data together
         * with the packet count and any warnings that were generated.
         * 
         * If timeout is FOREVER, fetchData() will wait indefinitely for
         * data to arrive. Otherwise it will wait timeout milliseconds,
         * and throw a timeout_exception if no data arrives in that time.
         * 
         * If packetsToRead is ALL_PACKETS, fetchData() will read all data
         * that arrives in the current buffer. Otherwise it will read 
         * up to packetsToRead data packets at a time and leave any remaining
         * data for the next call to fetchData().
         * 
         * If the SessionHandler is configured to exclude idle packets, idle
         * packets are excluded from the data blob's data vector, but included
         * in the packet count.
         * 
         * @note This function is not thread-safe. Do not call fetchData()
         * concurrently from two different threads, even if they are using
         * different devices.
         * 
         * @param timeout The maximum time to wait for data to arrive, in
         * milliseconds. If timeout is FOREVER, fetchData() will wait
         * indefinitely for data to arrive.
         * 
         * @param packetsToRead The number of packets to read in this call to
         * fetchData(). If packetsToRead is ALL_PACKETS, all packets in
         * the current buffer will be read.
         * 
         * @return A DataBlob containing the data, packet count, and any 
         * warnings that were generated.
         * 
         * @throws std::runtime_error if an error occurred that prevented
         * the function from reading data.
         * @throws timeout_exception if data could not be fetched within
         * timeout milliseconds.
         */
        virtual DataBlob fetchData(
            std::chrono::milliseconds timeout = FOREVER,
            int packetsToRead = ALL_PACKETS
        );

        /**
         * @brief Sets whether the session handler should include idle words
         * when fetching packets. False by default.
         * 
         * @param discard True to include idle words, false to ignore them.
         */
        virtual void setIncludeIdleWords(bool discard);

        /**
         * @brief Gets a list of all network devices on the system. If no
         * devices could be found, returns an empty vector.
         * 
         * @return A vector of Device objects.
         * 
         * @throws std::runtime_error if an error occurred.
         */
        static std::vector<Device> getNetworkDevices();

        // TODO: Testing. Part of this refactoring effort is to establish
        //       a unit test suite.
        // TODO: Go through the interface and make sure it's good.
        // TODO: Make sure everything works on different operating systems. The
        //       example program will need portable file management
        // TODO: Make sure the interface is obvious

    private:

        class SessionHandler_impl;
        SessionHandler_impl *impl = nullptr;

    };

    // TODO: Prevent external instantiation
    /**
     * @brief Represents a blob of data fetched from a network device.
     */
    struct DataBlob {

        /**
         * @brief The number of packets in the data blob.
         */
        int packetCount;

        /**
         * @brief The data fetched from the network device.
         */
        std::vector<uint8_t> data;

        /**
         * @brief Any warnings that were generated during the fetch.
         */
        std::vector<std::string> warnings;

        /**
         * @brief Packs the data blob into a vector of 64-bit integers.
         */
        std::vector<uint64_t> pack();

    };

} // namespace DAQCap