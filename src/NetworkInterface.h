/**
 * @file NetworkInterface.h
 *
 * @brief Provides a C++-style interface for managing network devices.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <string>
#include <vector>
#include <DAQCapDevice.h>

#include "Packet.h"

namespace DAQCap {

    struct Packet;

    /**
     * @brief Gets a list of all network devices on the system.
     * 
     * @return A vector of pairs, where each pair contains first the name of
     * a device and second its description.
     */
    std::vector<Device> getDevices();

    /**
     * @brief Provides an interface for retrieving packets from a network
     * device.
     */
    class Listener {

    public:

        /**
         * @brief Constructs a listener object for the given device.
         * 
         * @throws std::runtime_error if the device does not exist or could not
         * be initialized.
         */
        Listener(const std::string &deviceName);
        ~Listener();

        /**
         * @brief Interrupts calls to listen().
         */
        void interrupt();

        /**
         * @brief Waits for packets to arrive on the network device associated
         * with this Listener, then reads them into a vector of Packet objects
         * until packetsToRead packets have been read or the current buffer
         * is exhausted.
         * 
         * @param packetsToRead The number of packets to read. If packetsToRead
         * is -1, all packets in the current buffer are read. packetsToRead 
         * should not be zero or less than -1.
         */
        std::vector<Packet> listen(int packetsToRead);

    private:

        /*
         * NOTE: We go for the PIMPL idiom here to allow this header to serve
         *       as a common interface for multiple implementations. In
         *       particular, this will allow us to substitute the PCap library
         *       with a different library or with a mock implementation without
         *       disturbing the interface or needing to recompile code that 
         *       relies on it.
         */
        class Listener_impl;
        Listener_impl *impl = nullptr;

    };

}