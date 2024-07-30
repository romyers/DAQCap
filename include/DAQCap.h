/**
 * @file DAQCap.h
 *
 * @brief Main interface for the DAQCap library.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#pragma once


#include "DAQBlob.h"

#include <string>
#include <chrono>
#include <stdexcept>
#include <istream>

/**
 * @brief The library version.
 */
#define DAQCAP_VERSION "1.0.0"

namespace DAQCap {

	const int ALL_PACKETS = -1;
	const std::chrono::seconds FOREVER(-1);

	/**
	 * @brief Checks if Device::interrupt() calls are supported.
	 * 
	 * Device::interrupt() calls are only supported for Linux and Windows and
	 * for libpcap version 1.10.0 or later.
	 */
	bool interrupt_supported();

	/**
	 * @brief Represents a network device.
	 * 
	 * @note Devices are not client-instantiable. Access them via
	 * Device::getAllDevices() or Device::getDevice().
	 */
	class Device {

	public:

		/**
		 * @brief Gets a list of all available devices. Devices with the same
		 * name are the same device instance.
		 * 
		 * If the devices could not be fetched, returns an empty vector.
		 * 
		 * @note Do not delete the pointers returned by this function.
		 */
		static std::vector<Device*> getAllDevices();

		/**
		 * @brief Gets a device by name if it exists. If the device could not
		 * be found, returns nullptr.
		 * 
		 * @note Do not delete the pointer returned by this function.
		 */
		static Device *getDevice(const std::string &name);

		/**
		 * @brief Gets the name of the device.
		 */
		virtual std::string getName() const = 0;

		/**
		 * @brief Gets the description of the device.
		 */
		virtual std::string getDescription() const = 0;

		/**
		 * @brief Opens the device for data capture. If the device is already
		 * open, or fails to open for any reason, has no effect.
		 */
		virtual void open() = 0;

		/**
		 * @brief Checks if the device is open for data capture.
		 */
		virtual bool is_open() const = 0;

		/**
		 * @brief Closes a data capture session on the device, interrupting any
		 * concurrent calls to fetchData().
		 */
		virtual void close() = 0;

		/**
		 * @brief Interrupts any concurrent calls to fetchData() and forces
		 * them to return.
		 * 
		 * @note This function is supported for Linux and Windows and for
		 * libpcap versions 1.10.0 and later. For other platforms or versions,
		 * this function will have no effect, and it will be impossible
		 * to unblock fetchData() calls until data is received.
		 */
		virtual void interrupt() = 0;

		/**
		 * @brief Fetches data from the device.
		 * 
		 * Blocks until data is available on the device or the timeout is
		 * reached, then returns up to packetsToRead packets of data as
		 * a DataBlob.
		 * 
		 * If the timeout is reached, aborts the data read and returns
		 * an empty blob.
		 * 
		 * fetchData() may not be called concurrently, even from different
		 * Device instances.
		 * 
		 * @param timeout The maximum time to wait for data to be available
		 * before returning.
		 * 
		 * @param packetsToRead The maximum number of packets to read from the
		 * device in one call to fetchData. FetchData will return at most
		 * packetsToRead packets of data.
		 * 
		 * @return A DataBlob containing the raw packet data together with
		 * the number of packets in the blob and any warnings or errors that
		 * occurred.
		 * 
		 * @throws std::runtime_error if an error occurs while fetching data.
		 * 
		 * @note Timeouts are currently tested and supported on Linux only.
		 * They may work on MacOS, though this is untested. Timeouts will
		 * have no effect on windows.
		 */
		virtual DataBlob fetchData(
			std::chrono::seconds timeout = FOREVER,
			int packetsToRead = ALL_PACKETS
		) = 0;

		virtual ~Device() = default;

		Device(const Device &other) = delete;
		Device &operator=(const Device &other) = delete;

	protected:

		Device() = default;

	};

} // namespace DAQCap