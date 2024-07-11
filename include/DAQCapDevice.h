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

} // namespace DAQCap