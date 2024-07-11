/**
 * @file NetworkInterface.h
 *
 * @brief Provides a flexible interface for managing network devices.
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
         * @brief Interrupts calls to listen().
         */
        virtual void interrupt() = 0;

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
        virtual std::vector<Packet> listen(int packetsToRead) = 0;

        /**
         * @brief Constructs a listener object for the given device.
         * 
         * @throws std::runtime_error if the device does not exist or could not
         * be initialized.
         */
        static Listener *create(const std::string &deviceName);

    };

} // namespace DAQCap