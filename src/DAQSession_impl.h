/**
 * @file DAQSession_impl.h
 *
 * @brief Implements DAQCap::SessionHandler.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <DAQCap.h>

#include "Packet.h"

namespace DAQCap {

    /**
     * @brief Implementation class for DAQCap::SessionHandler. See
     * DAQCap::SessionHandler in DAQCap.h for documentation.
     */
    class SessionHandler::SessionHandler_impl {

    public:

        SessionHandler_impl();
        ~SessionHandler_impl();

        std::vector<std::shared_ptr<Device>> getAllNetworkDevices(
            bool reload = false
        );

        std::shared_ptr<Device> getNetworkDevice(const std::string &name);

        void startSession(const std::shared_ptr<Device> device);

        void endSession();

        void interrupt();
        
        DataBlob fetchData(
            std::chrono::milliseconds timeout, 
            int packetsToRead
        );

        void includeIdleWords(bool discard);

    private:

        std::unique_ptr<class NetworkManager> netManager = nullptr;

        bool includingIdleWords = false;

        Packet lastPacket;

        // Buffer for unfinished data words at the end of a packet
        std::vector<uint8_t> unfinishedWords;

        std::vector<std::shared_ptr<Device>> networkDeviceCache;

    };

}