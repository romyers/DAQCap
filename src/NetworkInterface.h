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
     * @return A vector of populated Device objects.
     */
    std::vector<Device> getDevices();

    /**
     * @brief Provides an interface for retrieving packets from a network
     * device.
     */
    class Listener {

    public:

        /**
         * @brief Interrupts calls to listen(), causing them to abort
         * execution and return. No effect if no calls to listen() are
         * currently executing.
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
         * 
         * @return A vector of Packet objects containing the read packets. If
         * interrupt() was called, the vector will be empty.
         * 
         * @throws std::runtime_error if an error occurred.
         */
        virtual std::vector<Packet> listen(int packetsToRead) = 0;

        /**
         * @brief Constructs a listener object for the given device.
         * 
         * @param device The device to listen on.
         * 
         * @throws std::runtime_error if the device does not exist or could not
         * be initialized.
         */
        static Listener *create(const Device &device);

    };

} // namespace DAQCap