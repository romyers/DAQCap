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
#include <fstream>

#include <sys/stat.h>

#include <DAQCap.h>

DAQCap::Device runDeviceSelection();

bool        directoryExists    (const std::string &path         );
bool        createDirectory    (const std::string &path         );
std::string getCurrentTimestamp(const std::string &format       );

int main(int argc, char **argv) {

    ///////////////////////////////////////////////////////////////////////////
    // List available network devices and select one to listen on
    ///////////////////////////////////////////////////////////////////////////

    DAQCap::Device device;
    try {

        device = runDeviceSelection();   

    } catch(const std::exception &e) {

        std::cout << e.what() << std::endl;

        exit(1);

    }
    std::cout << "Listening on device: " << device.name << std::endl;

    ///////////////////////////////////////////////////////////////////////////
    // Initialize a SessionHandler for the selected device
    ///////////////////////////////////////////////////////////////////////////

    // TODO: Cleaner way to do this that doesn't require heap allocation
    DAQCap::SessionHandler *handler;

    try {

        handler = new DAQCap::SessionHandler(device);

    } catch(const std::exception &e) {

        std::cerr << e.what() << std::endl;
        std::cout << "Aborted run!" << std::endl;

        exit(1);

    }

    ///////////////////////////////////////////////////////////////////////////
    // Set up output file
    ///////////////////////////////////////////////////////////////////////////

    std::string runTimestamp = getCurrentTimestamp("%Y%m%d_%H%M%S");

	std::cout << std::endl << "Starting run: " << runTimestamp << std::endl; 

    // TODO: Don't hardcode this directory path
    if(!directoryExists("../data")) {

        createDirectory("../data");

        std::cout << "Created output directory: ../data" << std::endl;

    }

	std::string outputFile("../data/run_");
	outputFile += runTimestamp;
	outputFile += ".dat";

	std::ofstream fileWriter(outputFile);
	if(!fileWriter.is_open()) {

		std::cerr << "Failed to open output file: " << outputFile << std::endl;
		std::cout << "Aborted run!" << std::endl;

		exit(1);

	}

	std::cout << "Saving packet data to: " << outputFile << std::endl;

    ///////////////////////////////////////////////////////////////////////////
    // Fetch packets and write to file
    ///////////////////////////////////////////////////////////////////////////

	int packets   = 0;
	int thousands = 0;

    // TODO: Stop condition and interrupt signal
	while(true) {

        DAQCap::DataBlob blob = handler->fetchPackets(1000);

        std::cout << "Cycle" << std::endl;
        std::cout << blob.countPackets() << " packets fetched" << std::endl;

		fileWriter.write((char*)blob.data.data(), blob.data.size());
		fileWriter.flush();

		packets += blob.countPackets();

		while(packets / 1000 > thousands) {

			++thousands;
			std::cout << "Recorded " << thousands * 1000 << " packets";

		}

	}

    ///////////////////////////////////////////////////////////////////////////
    // Cleanup
    ///////////////////////////////////////////////////////////////////////////

	fileWriter.close();
	std::cout << std::endl << "Data capture finished!" << std::endl;
	std::cout << packets << " packets recorded." << std::endl;
    
    delete handler;

    return 0;

}

DAQCap::Device runDeviceSelection() {

    std::vector<DAQCap::Device> devices 
        = DAQCap::SessionHandler::getDeviceList();

    if(devices.size() == 0) {

        std::cout << "No network devices found. Check your permissions." 
                  << std::endl;
        exit(0);

    }

    std::cout << std::endl << "Available network devices:" << std::endl;
    for(int i = 0; i < devices.size(); ++i) {

        std::cout << i + 1 << ": " 
                  << devices[i].name << "    " 
                  << devices[i].description << std::endl;

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