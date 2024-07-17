/**
 * @file FetchDevice.h
 *
 * @brief TODO: Write
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once

#include <DAQCapDevice.h>

#include <string>
#include <memory>
#include <chrono>

namespace DAQCap {

    class FetchDevice : public Device {

    public:

        virtual void close();

        virtual void fetchData(
            std::chrono::milliseconds timeout,
            int packetsToRead
        );

        FetchDevice(const Device &other) = delete;
        FetchDevice &operator=(const Device &other) = delete;

    protected:

        FetchDevice() = default;

    };

} // namespace DAQCap