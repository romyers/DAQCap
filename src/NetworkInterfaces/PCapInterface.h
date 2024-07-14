/**
 * @file PCapInterface.h
 *
 * @brief NetworkManager implementation for libpcap.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <NetworkInterface.h>

typedef struct pcap pcap_t;

namespace DAQCap {

    /**
     * @brief Implementation of NetworkManager for libpcap. See 
     * NetworkInterface.h::NetworkManager for documentation.
     */
    class PCap_Manager: public NetworkManager {

    public:

        PCap_Manager() = default;
        virtual ~PCap_Manager();

        // NOTE: getAllDevices() could be decoupled from everything else for 
        //       PCap, but we keep it coupled in case we implement 
        //       another network library that doesn't allow that
        //       separation.

        virtual std::vector<Device> getAllDevices() override;
        virtual void startSession(const Device &device) override;
        virtual void endSession() override;

        virtual bool hasOpenSession() override;

        PCap_Manager(const PCap_Manager &other)            = delete;
        PCap_Manager &operator=(const PCap_Manager &other) = delete;

        virtual void interrupt() override;
        virtual std::vector<Packet> fetchPackets(int packetsToRead) override;

    private:

        pcap_t *handler = nullptr;

    };

}

