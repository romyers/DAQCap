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

#include <DAQCapDevice.h>

// TODO: Documentation, especially throws

// TODO: Idea -- we can use CMake to pass in a flag that indicates whether we
//       have pcap. If we don't, we can use the flag to skip the include and
//       stub the network interface, then flag the library as disabled via
//       the public interface.

namespace DAQCap {

    struct DataBlob;

    // TODO: A better way to do this
    const int NO_LIMIT = -1;

    /**
     * @brief Exception thrown to signal that a timeout occurred.
     */
    class timeout_exception : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    /**
     * @brief Manages a session with a network device. 
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
         * @brief Thread-safe method that interrupts calls to fetchPackets().
         */
        virtual void interrupt();

        /**
         * @brief Waits for packets to arrive on the network device associated
         * with this SessionHandler, then reads them into a DataBlob until
         * packetsToRead packets have been read or all packets in the current
         * buffer have been read, whichever comes first. Idle packets are
         * excluded from the data blob's data vector, but included in the
         * bufferedPackets count.
         * 
         * @param timeout The maximum time to wait for packets to arrive, in
         * milliseconds. If timeout is NO_LIMIT, fetchPackets() will wait
         * indefinitely for packets to arrive.
         * 
         * @param packetsToRead The number of packets to read in this call to
         * fetchPackets(). If packetsToRead is NO_LIMIT, all packets in
         * the current buffer will be read.
         * 
         * @return A DataBlob containing the packets together with their packet
         * numbers. If interrupt() was called, the DataBlob will be empty.
         * 
         * @throws std::runtime_error if an error occurred.
         * @throws interrupt_exception if the call was interrupted.
         */
        virtual DataBlob fetchPackets(
            int timeout = NO_LIMIT,      // milliseconds
            int packetsToRead = NO_LIMIT // packets
        );

        /**
         * @brief Gets a list of all network devices on the system.
         * 
         * @return A vector of Device objects.
         */
        static std::vector<Device> getDeviceList();

        // TODO: Add functionality for:
        //         -- checking packet numbers
        //         -- removing idle words
        // TODO: Testing. Part of this refactoring effort is to establish
        //       a unit test suite.
        // TODO: Go through the interface and make sure it's good.
        // TODO: Make sure everything works on different operating systems. The
        //       example program will need portable file management

    private:

        class SessionHandler_impl;
        SessionHandler_impl *impl = nullptr;

    };

    /**
     * @brief Represents data captured from a network device.
     */
    struct DataBlob {

        /**
         * @brief The data stored in this blob.
         */
        std::vector<unsigned char> data;

        /**
         * @brief The packet numbers of the packets stored in this blob.
         */
        std::vector<int> packetNumbers;

        /**
         * @brief Returns number of packets stored in this blob.
         * 
         * @return The number of packets stored in this blob.
         */
        virtual int countPackets() const;

    };

} // namespace DAQCap