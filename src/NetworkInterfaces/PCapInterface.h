/**
 * @file PCapInterface.h
 *
 * @brief NetworkInterface implementation for libpcap.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <NetworkInterface.h>
#include <DAQCapDevice.h>

typedef struct pcap pcap_t;

namespace DAQCap {

    /**
     * @brief Implementation of a DAQCap::Device for libpcap. See
     * DAQCapDevice.h::Device for documentation.
     */
    class PCapDevice : public Device {

    public:

        PCapDevice(std::string name, std::string description);

        virtual ~PCapDevice() = default;

        virtual std::string getName() const override;
        virtual std::string getDescription() const override;


    private:

        std::string name;
        std::string description;

    };

    /**
     * @brief Implementation of NetworkManager for libpcap. See 
     * NetworkInterface.h::NetworkManager for documentation.
     */
    class PCapManager: public NetworkManager {

    public:

        PCapManager();
        virtual ~PCapManager();

        // NOTE: getAllDevices() could be decoupled from everything else for 
        //       PCap, but we keep it coupled in case we implement 
        //       another network library that doesn't allow that
        //       separation.

        virtual std::vector<std::shared_ptr<Device>> getAllDevices() override;
        virtual void startSession(const std::shared_ptr<Device> device) override;
        virtual void endSession() override;

        PCapManager(const PCapManager &other)            = delete;
        PCapManager &operator=(const PCapManager &other) = delete;

        virtual void interrupt() override;
        virtual std::vector<Packet> fetchPackets(int packetsToRead) override;

    private:

        pcap_t *handler = nullptr;

    };

}

