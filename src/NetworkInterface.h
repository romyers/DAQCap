/**
 * @file NetworkInterface.h
 *
 * @brief Provides a flexible interface for managing network devices.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include "Packet.h"

#include <DAQCapDevice.h>

#include <string>
#include <vector>

namespace DAQCap {

    struct Packet;

    /**
     * @brief Provides an interface for retrieving packets from a network
     * device.
     */
    class NetworkManager {

    public:

        virtual ~NetworkManager() = default;

        NetworkManager(const NetworkManager &other) = delete;
        NetworkManager &operator=(const NetworkManager &other) = delete;

        /**
         * @brief Gets a list of all network devices on the system.
         * If no devices could be found, returns an empty vector.
         * 
         * @return A vector of populated Device pointers.
         * 
         * @throws std::runtime_error if an error occurred.
         */
        virtual std::vector<std::shared_ptr<Device>> getAllDevices() = 0;

        /**
         * @brief Returns true if a session is currently open, false otherwise.
         */
        virtual bool hasOpenSession() = 0;

        /**
         * @brief Begins a capture session on the specified device, and prepares
         * the NetworkManager to fetch data from it.
         * 
         * @param device The device to start a session on.
         * 
         * @throws std::runtime_error if the device could not be initialized.
         * @throws std::logic_error if a session is already open.
         */
        virtual void startSession(const std::shared_ptr<Device> device) = 0;

        /**
         * @brief If a capture session is in progress, ends it. Otherwise
         * does nothing.
         */
        virtual void endSession() = 0;

        /**
         * @brief Interrupts calls to fetchPackets(), causing them to abort
         * execution and return. No effect if no calls to fetchPackets() are
         * currently executing.
         * 
         * @note This function is thread-safe.
         */
        virtual void interrupt() = 0;

        /**
         * @brief Waits for packets to arrive on the network device associated
         * with this NetworkManager, then reads them into a vector of Packet objects
         * until packetsToRead packets have been read or the current buffer
         * is exhausted.
         * 
         * @note This function is not necessarily threadsafe.
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
        virtual std::vector<Packet> fetchPackets(int packetsToRead) = 0;

        /**
         * @brief Constructs a NetworkManager object.
         * 
         * @throws std::invalid_argument if the device does not exist.
         * @throws std::runtime_error if the device could not
         * be initialized.
         */
        static std::unique_ptr<NetworkManager> create();

    protected:

        NetworkManager() = default;

    };

} // namespace DAQCap