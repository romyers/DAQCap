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

// TODO: Ask Yuxiang about the phase 1 packet format. I'd like to be able to
//       support phase 1 and phase 2 systems.

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
     * The SessionHandler class provides a high-level interface for interacting
     * with and reading from network devices.
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

        /**
         * @brief Gets a network device by name.
         * 
         * @note This function caches the list of devices on the first call,
         * and returns the cached list on subsequent calls. To refresh the
         * list of devices, call clearDeviceCache().
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
         * @brief Clears the internal cache of network devices.
         */
        void clearDeviceCache();

        /**
         * @brief Gets a list of all network devices on the system. If no
         * devices could be found, returns an empty vector.
         * 
         * @note This function caches the list of devices on the first call,
         * and returns the cached list on subsequent calls. To refresh the
         * list of devices, call clearDeviceCache().
         * 
         * @return A vector of Device pointers.
         * 
         * @throws std::runtime_error if an error occurred.
         */
        std::vector<std::shared_ptr<Device>> getAllNetworkDevices();

        /**
         * @brief Begins a capture session on the specified device, and prepares
         * the SessionHandler to fetch data from it.
         * 
         * @note Calling startSession() on a SessionHandler that is already
         * in the middle of a session will close the current session and start
         * a new one. This will interrupt any concurrent calls to fetchData().
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
         * with the packet count and any warnings that were generated. Any idle
         * data words are excluded.
         * 
         * If timeout is FOREVER, fetchData() will wait indefinitely for
         * data to arrive. Otherwise it will abort with a timeout_exception
         * if no data arrives within the specified time.
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
         * REQUIRES: 
         *   - timeout >= 0 || timeout == FOREVER
         *   - packetsToRead >= 0 || packetsToRead == ALL_PACKETS
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

        SessionHandler(const SessionHandler &other)            = delete;
        SessionHandler &operator=(const SessionHandler &other) = delete;

    private:

        std::unique_ptr<class NetworkManager> netManager;

        std::unique_ptr<class Packet> lastPacket;

        std::vector<std::shared_ptr<Device>> networkDeviceCache;

        // Buffer for unfinished data words at the end of a packet
        std::vector<uint8_t> unfinishedWords;

    };

} // namespace DAQCap