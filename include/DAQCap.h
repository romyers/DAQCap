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

    extern const int ALL_PACKETS;
    extern const std::chrono::milliseconds FOREVER;

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
    class SessionHandler final {

    public:

        /**
         * @brief Constructs a SessionHandler objec.
         */
        SessionHandler();
        ~SessionHandler();

        // TODO: Perhaps a reloadDeviceCache() method would be useful.
        /**
         * @brief Gets a network device by name.
         * 
         * @note If the device is not in the internal cache, the cache will be
         * reloaded. But if a device in the internal cache has since been 
         * removed, the cache will not be reloaded, and the device will
         * still be returned. If an up-to-date cache is required, call
         * getAllNetworkDevices() with the reload parameter set to true.
         * 
         * @param name The name of the device to get.
         * 
         * @return A pointer to the device with the specified name, or a null
         * device if the device could not be found.
         * 
         * @throws std::runtime_error if an error occurred.
         */
        std::shared_ptr<Device> getNetworkDevice(const std::string &name);

        /**
         * @brief Gets a list of all network devices on the system. If no
         * devices could be found, returns an empty vector.
         * 
         * @param reload Whether to reload internally cached network devices.
         * By default, the internal cache is not reloaded, and the device
         * list will not be updated for any newly added or removed devices.
         * 
         * @return A vector of Device pointers.
         * 
         * @throws std::runtime_error if an error occurred.
         */
        std::vector<std::shared_ptr<Device>> getAllNetworkDevices(
            bool reload = false
        );

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
        void startSession(const std::shared_ptr<Device> device);

        /**
         * @brief Ends a capture session on the current device.
         */
        void endSession();

        /**
         * @brief Thread-safe method that interrupts calls to fetchData().
         */
        void interrupt();

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
        DataBlob fetchData(
            std::chrono::milliseconds timeout = FOREVER,
            int packetsToRead = ALL_PACKETS
        );

        /**
         * @brief Sets whether the session handler should include idle words
         * when fetching packets. False by default.
         * 
         * @param discard True to include idle words, false to ignore them.
         */
        void includeIdleWords(bool discard);

        SessionHandler(const SessionHandler &other)            = delete;
        SessionHandler &operator=(const SessionHandler &other) = delete;

    private:

        class SessionHandler_impl;
        SessionHandler_impl *impl = nullptr;

    };

} // namespace DAQCap