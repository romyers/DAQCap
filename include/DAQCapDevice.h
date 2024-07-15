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

        Device(const Device &other) = delete;
        Device &operator=(const Device &other) = delete;

    protected:

        Device() = default;

    };

} // namespace DAQCap