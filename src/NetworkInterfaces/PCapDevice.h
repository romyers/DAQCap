/**
 * @file PCapInterface.h
 *
 * @brief Device implementation for libpcap.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <DAQCapDevice.h>

#include "PacketProcessor.h"

#include <string>
#include <chrono>

// TODO: Should we split some of the packet fetching out into a helper class?

typedef struct pcap pcap_t;

namespace DAQCap {

    class PCapDevice : public Device {

    public:

        PCapDevice(std::string name, std::string description);
        virtual ~PCapDevice() = default;

        virtual std::string getName() const override;
        virtual std::string getDescription() const override;

        virtual void open() override;
        virtual void close() override;

        virtual void interrupt() override;

        virtual DataBlob fetchData(
            std::chrono::milliseconds timeout = FOREVER,
            int packetsToRead = ALL_PACKETS
        ) override;

    private:

        std::string name;
        std::string description;

        pcap_t *handler;

    };

} // namespace DAQCap