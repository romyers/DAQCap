/**
 * @file DAQCap.h
 *
 * @brief Main interface for the DAQCap library.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <DAQCapDevice.h>
#include <DAQBlob.h>

#include <vector>
#include <stdexcept>
#include <string>
#include <chrono>

namespace DAQCap {

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
         * @brief Constructs a SessionHandler objec.
         */
        SessionHandler();
        virtual ~SessionHandler();

        SessionHandler(const SessionHandler &other)            = delete;
        SessionHandler &operator=(const SessionHandler &other) = delete;

        /**
         * @brief Gets a network device by name.
         * 
         * @param name The name of the device to get.
         * 
         * @return The Device object with the specified name, or a null
         * device if an error occurred or the device could not be found.
         */
        Device getNetworkDevice(const std::string &name);

        /**
         * @brief Gets a list of all network devices on the system. If no
         * devices could be found, returns an empty vector.
         * 
         * @return A vector of Device objects.
         * 
         * @throws std::runtime_error if an error occurred.
         */
        std::vector<Device> getAllNetworkDevices();

        /**
         * @brief Begins a capture session on the specified device, and prepares
         * the SessionHandler to fetch data from it.
         * 
         * @note Calling startSession() on a SessionHandler that is already
         * in the middle of a session will close the current session and start
         * a new one.
         * 
         * @throws std::runtime_error if the device does not exist or could not
         * be initialized.
         */
        void startSession(const Device &device);

        /**
         * @brief Ends a capture session on the current device.
         */
        void endSession();

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
         * @note A session must have been started with startSession() before
         * calling fetchData().
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
         * @throws std::logic_error if a session has not been started.
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
        virtual void includeIdleWords(bool discard);

    private:

        class SessionHandler_impl;
        SessionHandler_impl *impl = nullptr;

    };

} // namespace DAQCap