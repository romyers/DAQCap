/**
 * @file DAQCapDevice.h
 *
 * @brief Provides a struct for representing network devices.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <string>
#include <memory>

namespace DAQCap {

    const int ALL_PACKETS = -1;
    const std::chrono::milliseconds FOREVER(-1);

    /**
     * @brief Represents a network device on the system.
     * 
     * @note Devices are not meant to be instantiated or subclassed by
     * external code. Use SessionHandler::getNetworkDevice() or
     * SessionHandler::getAllNetworkDevices() to access them.
     */
    class Device {

    public:

        virtual ~Device() = default;

        /**
         * @brief Get the name of the device.
         */
        virtual std::string getName() const = 0;

        /**
         * @brief Get the description of the device.
         */
        virtual std::string getDescription() const = 0;

        virtual void open() = 0;
        virtual void close() = 0;

        virtual void interrupt() = 0;

        virtual DataBlob fetchData(
            std::chrono::milliseconds timeout = FOREVER,
            int packetsToRead = ALL_PACKETS
        ) = 0;

        Device(const Device &other) = delete;
        Device &operator=(const Device &other) = delete;

    protected:

        Device() = default;

    };

} // namespace DAQCap