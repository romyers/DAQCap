/**
 * @file p2ecap_standalone.cpp
 *
 * @brief Simple use-case for the DAQCap library. This program reads miniDAQ
 * data from an ethernet device and saves it to a file.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#include <iostream>
#include <cstring>

#include <sys/stat.h>

#include <DAQCap.h>

DAQCap::Device runDeviceSelection();

bool        directoryExists    (const std::string &path         );
bool        createDirectory    (const std::string &path         );
std::string getCurrentTimestamp(const std::string &format       );
void        createIfMissing    (const std::string &directoryName);

int main(int argc, char **argv) {

    DAQCap::Device device;

    try {

        device = runDeviceSelection();   

    } catch(const std::exception &e) {

        std::cout << e.what() << std::endl;

        exit(1);

    }

    std::cout << "Listening on device: " << device.name << std::endl;

    try {

        // TODO: This needs to be a loop

        DAQCap::SessionHandler handler(device);

        DAQCap::DataBlob blob = handler.fetchPackets();

        std::cout << blob.countPackets() << std::endl;

    } catch(const std::exception &e) {

        std::cerr << e.what() << std::endl;
        std::cout << "Aborted run!" << std::endl;

        exit(1);

    }
    
    // Finally, we need to write captured data to a file
    std::cout << "TODO: Implement" << std::endl;

    return 0;

}

DAQCap::Device runDeviceSelection() {

    std::vector<DAQCap::Device> devices = DAQCap::getDeviceList();

    std::cout << std::endl << "Available network devices:" << std::endl;
    for(int i = 0; i < devices.size(); ++i) {

        std::cout << i + 1 << ": " 
                  << devices[i].name << "    " 
                  << devices[i].description << std::endl;

    }

    if(devices.size() == 0) {

        std::cout << "No network devices found. Check your permissions." 
                  << std::endl;
        exit(0);

    }

    int deviceNum = -1;
    do {

        std::cout << "Select a device (1-" << devices.size() << "): ";
        std::string selection;
        std::cin >> selection;

        try {

            deviceNum = std::stoi(selection);

        } catch(std::invalid_argument& e) {

            std::cout << "Invalid selection." << std::endl;
            continue;

        }

        if(deviceNum < 1 || deviceNum > devices.size()) {

            std::cout << "Invalid selection." << std::endl;

        } else {

            break;

        }

    } while(true);

    return devices[deviceNum - 1];

}

bool directoryExists(const std::string &path) {

	struct stat sb;

	if(stat(path.data(), &sb) == 0) {

		return true;

	}

	return false;

}

bool createDirectory(const std::string &path) {

	if(mkdir(path.data(), 0777) == 0) return true;

	return false;

}

std::string getCurrentTimestamp(const std::string &format) {

	char formatBuffer[40];
	time_t sys_time;
	struct tm *timeinfo;
	sys_time = time(0);
	timeinfo = localtime(&sys_time);
	std::memset(formatBuffer, 0, sizeof(formatBuffer));
	strftime(formatBuffer, 40, format.data(), timeinfo);

	return std::string(formatBuffer);

}

void createIfMissing(const std::string &directoryName) {

	if(!directoryExists(directoryName)) {

		createDirectory(directoryName);

		std::cout << "Created output directory: " 
                  << directoryName 
                  << std::endl;

	}

}