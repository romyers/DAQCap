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

namespace DAQCap {

    /**
     * @brief Represents a network device on the system.
     */
    class Device {

    public:

        /**
         * @brief Creates a null device.
         */
        Device();

        /**
         * @brief Creates a device with the given name and description.
         */
        Device(std::string name, std::string description);

        /**
         * @brief Get the name of the device.
         */
        std::string getName() const;

        /**
         * @brief Get the description of the device.
         */
        std::string getDescription() const;

        /**
         * @brief Implicit conversion to bool.
         */
        explicit operator bool() const;

    private:

        std::string name;
        std::string description;

    };

} // namespace DAQCap