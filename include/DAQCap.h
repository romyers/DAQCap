/**
 * @file DAQCap.h
 *
 * @brief Main interface for the DAQCap library.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <string>
#include <ostream>
#include <vector>

// TODO: Can we move this include out?
#include <pcap.h>

// TODO: Documentation, especially throws

// TODO: Decide how to handle copy constructor/assignment

namespace DAQCap {

    /**
     * @brief Represents a network device on the system.
     */
    struct Device {

        /**
         * @brief The device name.
         */
        std::string name;

        /**
         * @brief The device description.
         */
        std::string description;

    };

    /**
     * @brief Represents data captured from a network device.
     */
    struct DataBlob {

        /**
         * @brief The number of packets lost while assembling this blob.
         */
        int lostPackets = 0;

        /**
         * @brief The number of packets buffered in this blob.
         */
        int bufferedPackets = 0;

        // TODO: Try to get rid of these
        std::string statusMessage;
        std::vector<std::string> errorMessages;

        /**
         * @brief The data stored in this blob.
         */
        std::vector<unsigned char> data;

    };

    /**
     * @brief Provides an interface for managing network devices.
     */
    class SessionHandler {

    public:

        SessionHandler(const Device &device, size_t wordSize = 5);
        ~SessionHandler();

        SessionHandler(const SessionHandler &other)            = delete;
        SessionHandler &operator=(const SessionHandler &other) = delete;

        /**
         * @brief Interrupts any currently running calls to fetchPackets().
         */
        void interrupt();

        /**
         * @brief Fetches packets from the session.
         * 
         * @param checkPackets If true, the function will check to see if any
         * packets were lost.
         * 
         * @return A DataBlob containing the packets.
         */
        DataBlob fetchPackets(bool checkPackets = true);

    private:

        pcap_t *handler;

        int lastPacket;

        size_t wordSize;

    };

    /**
     * @brief Gets a list of all network devices on the system.
     * 
     * @return A vector of Device objects.
     */
    std::vector<Device> getDevices();

}