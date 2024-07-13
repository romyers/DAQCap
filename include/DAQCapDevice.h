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

// TODO: In order to support other network interfaces, we need to support
//       other device formats. This will require a more flexible Device
//       class -- probably another factory pattern.
//         -- If we do this, device should go with the network interface
//              -- Except that devices are exposed to the user, so they
//                 should be in the public interface. We need to balance
//                 exposing Device to client code with keeping
//                 interface details behind NetworkInterface.

namespace DAQCap {

    // TODO: Prevent external instantiation
    //         -- If we make it abstract, we can keep the create() method in
    //            subclasses.
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

        /**
         * @brief Implicit conversion to bool.
         */
        explicit operator bool() const {
            return !name.empty();
        }

    };

} // namespace DAQCap