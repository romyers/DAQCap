/**
 * @file p2ecap_standalone.cpp
 *
 * @brief A standalone program that captures miniDAQ data from a network device
 * and writes it to a file.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#include <iostream>
#include <cstring>
#include <fstream>
#include <mutex>
#include <future>

#include <getopt.h>

#include <DAQCap.h>

// Holds the command-line arguments
struct Arguments {

    // Output directory path
    std::string outPath;

    // Name of the network device to listen on
    std::string deviceName;

    // Whether the help option was specified
    bool help = false;

    /// Whether valid arguments were specified
    bool valid = true;

    // The maximum number of packets to capture
    int maxPackets = std::numeric_limits<int>::max();

};

// Parses command-line arguments
Arguments parseArguments(int argc, char **argv);

// Prompt user to select a device from a list
DAQCap::Device promptForDevice(const std::vector<DAQCap::Device> &devices);

// Get a timestamp representing the current time in the given format
std::string getCurrentTimestamp(const std::string &format);

// Print a list of available network devices
void printDeviceList(
    std::ostream &os, 
    const std::vector<DAQCap::Device> &devices
);

// Print the help message
void printHelp(std::ostream &os);

int main(int argc, char **argv) {

    ///////////////////////////////////////////////////////////////////////////
    // Parse CL arguments and handle help/invalid
    ///////////////////////////////////////////////////////////////////////////

    Arguments args = parseArguments(argc, argv);

    if(!args.valid || args.help) {

        printHelp(std::cout);

        return 0;

    }


    ///////////////////////////////////////////////////////////////////////////
    // Select a device to listen on
    ///////////////////////////////////////////////////////////////////////////

    DAQCap::Device device;
    device.name = args.deviceName;

    // If no device was specified at the command line, have the user choose one
    // from the available devices
    if(args.deviceName.empty()) {

        std::vector<DAQCap::Device> devices;

        try {

            devices = DAQCap::SessionHandler::getNetworkDevices();

        } catch(const std::exception &e) {

            std::cerr << e.what() << std::endl;
            std::cout << "Exiting..." << std::endl;

            return 1;

        }

        if(devices.empty()) {

            std::cout << "No network devices found. Check your permissions."
                      << std::endl;

        }

        // Print the list of devices
        printDeviceList(std::cout, devices);

        // Prompt the user to select a device
        // TODO: std::optional for C++17
        device = promptForDevice(devices);

        // If we got a null device, exit
        if(!device) {

            std::cout << "No device selected. Exiting..." << std::endl;
            return 0;

        }

    }

    ///////////////////////////////////////////////////////////////////////////
    // Initialize a SessionHandler for the selected device
    ///////////////////////////////////////////////////////////////////////////

    std::unique_ptr<DAQCap::SessionHandler> handler;

    try {

        // Initialize the session handler
        handler = std::unique_ptr<DAQCap::SessionHandler>(
            new DAQCap::SessionHandler(device)
        );

    } catch(const std::exception &e) {

        std::cerr << e.what() << std::endl;
        std::cout << "Aborted run!" << std::endl;

        return 1;

    }

    ///////////////////////////////////////////////////////////////////////////
    // Set up output file
    ///////////////////////////////////////////////////////////////////////////

    std::string runLabel 
        = std::string("run_") + getCurrentTimestamp("%Y%m%d_%H%M%S");

    std::string outputFile = args.outPath;

    // If the output directory is nonempty and doesn't end in a slash, add one
    if(!outputFile.empty()) {

        if(outputFile.back() != '/' && outputFile.back() != '\\') {

            outputFile += '/';

        }

    }

    outputFile += runLabel + ".dat";

	std::ofstream fileWriter(outputFile);
	if(!fileWriter.is_open()) {

		std::cerr << "Failed to open output file: " << outputFile << std::endl;
        std::cerr << "Does the output directory exist?" << std::endl;
		std::cout << "Aborted run!" << std::endl;

		return 1;

	}

    std::cout << "Listening on device: " << device.name << std::endl;
	std::cout << "Starting run: " << runLabel << std::endl; 
	std::cout << "Saving packet data to: " 
              << outputFile 
              << std::endl;
    std::cout << "Enter 'q' to quit." << std::endl;
    std::cout << std::endl;

    ///////////////////////////////////////////////////////////////////////////
    // Asynchronously listen for user to quit
    ///////////////////////////////////////////////////////////////////////////

    // Makes this threadsafe
    std::atomic<bool> running(true);

    // NOTE: handler->interrupt() is threadsafe, so we don't need a mutex
    // NOTE: We have to store the future or it will be destroyed and the
    //       destructor will block until the thread finishes
    auto future = std::async(
        std::launch::async,
        [&handler, &running]() {

            std::string input;
            while(running) {

                std::getline(std::cin, input);

                if(input == "q" || input == "quit" || input == "exit") {

                    running = false;
                    handler->interrupt();

                }

            }

        }
    );

    ///////////////////////////////////////////////////////////////////////////
    // Fetch packets and write to file
    ///////////////////////////////////////////////////////////////////////////

	int packets   = 0;
	int thousands = 0;

	while(running) {

        DAQCap::DataBlob blob;
        try {

            blob = handler->fetchData(std::chrono::seconds(1));

        } catch(const DAQCap::timeout_exception &t) {

            std::cerr << "Timed out while waiting for packets." << std::endl;

            continue;

        } catch(const std::exception &e) {

            std::cerr << e.what() << std::endl;

            continue; 

        }

        for(const std::string &warning : blob.warnings) {

            std::cerr << warning << std::endl;

        }

		fileWriter.write((char*)blob.data.data(), blob.data.size());
		fileWriter.flush();

		packets += blob.packetCount;

		while(packets / 1000 > thousands) {

			++thousands;

			std::cout << "Recorded " << thousands * 1000 << " packets";

		}

        if(packets >= args.maxPackets) {

            running = false;

        }

	}

    ///////////////////////////////////////////////////////////////////////////
    // Cleanup
    ///////////////////////////////////////////////////////////////////////////

    std::cout << std::endl;
	std::cout << "Data capture finished!" << std::endl;
	std::cout << packets << " packets recorded." << std::endl;

    return 0;

}

DAQCap::Device promptForDevice(const std::vector<DAQCap::Device> &devices) {

    // Prompt user for a device
    int deviceNum = -1;
    std::string selection;
    do {

        std::cout << "Select a device (1-" << devices.size() << ")";
        std::cout << " or select 'q' to quit: ";

        std::getline(std::cin, selection);

        // Return an empty device if the user wants to quit
        if(selection == "q" || selection == "quit" || selection == "exit") {

            return DAQCap::Device();

        }

        try {

            deviceNum = std::stoi(selection);

        } catch(std::invalid_argument& e) {

            std::cout << "Invalid selection:" << selection << std::endl;
            continue;

        }

        if(deviceNum < 1 || deviceNum > devices.size()) {

            std::cout << "Invalid selection: " << selection << std::endl;

        } else {

            break;

        }

    } while(true);

    std::cout << std::endl;

    return devices[deviceNum - 1];

}

Arguments parseArguments(int argc, char **argv) {

    Arguments args;

    // Define arguments
    const char *shortOpts = "o:d:hm:";
    const struct option longOpts[] = {
        {"out", required_argument, nullptr, 'o'},
        {"device", required_argument, nullptr, 'd'},
        {"help", no_argument, nullptr, 'h'},
        {"max-packets", required_argument, nullptr, 'm'},
        {nullptr, 0, nullptr, 0}
    };

    // Handle arguments
    while(optind < argc) {

        int opt = getopt_long(argc, argv, shortOpts, longOpts, nullptr);

        if(opt == -1) break;

        switch(opt) {

            case 'o':
                args.outPath = optarg;
                break;

            case 'd':
                args.deviceName = optarg;
                break;

            case 'm':
                try {
                        
                    args.maxPackets = std::stoi(optarg);

                } catch(std::invalid_argument &e) {

                    std::cerr << "-m, --max-packets must take an integer"
                              << " argument." 
                              << std::endl;

                    args.valid = false;

                }
                break;

            case 'h':
                args.help = true;
                break;

            default:
                args.valid = false;

        }

    }

    return args;

}

std::string getCurrentTimestamp(const std::string &format) {

	time_t sys_time;
	struct tm *timeinfo;

	sys_time = time(0);
	timeinfo = localtime(&sys_time);

	char formatBuffer[40];

	std::memset(formatBuffer, 0, sizeof(formatBuffer));
	strftime(formatBuffer, 40, format.data(), timeinfo);

	return std::string(formatBuffer);

}

void printDeviceList(
    std::ostream &os, 
    const std::vector<DAQCap::Device> &devices
) {

    // Find the biggest name and description so we can format the output
    size_t paddingSize = 4;
    size_t biggestName = 0;
    size_t biggestFullText = 0;
    for(const DAQCap::Device &device : devices) {

        if(device.name.size() > biggestName) biggestName = device.name.size();

    }
    for(int i = 0; i < devices.size(); ++i) {

        size_t fullTextSize = biggestName 
            + paddingSize 
            + devices[i].description.size()
            + (i + 1) / 10 + 3;

        if(fullTextSize > biggestFullText) biggestFullText = fullTextSize;

    }

    // Print the devices
    std::cout << "Available network devices:" << std::endl;
    std::cout << std::string(
        biggestFullText, 
        '-'
    );
    std::cout << std::endl;
    for(int i = 0; i < devices.size(); ++i) {

        std::string padding(
            biggestName - devices[i].name.size() + paddingSize, 
            ' '
        );

        std::cout << i + 1 
                  << ": " 
                  << devices[i].name
                  << padding 
                  << devices[i].description 
                  << std::endl;

    }
    std::cout << std::string(
        biggestFullText, 
        '-'
    );
    std::cout << std::endl;

}

void printHelp(std::ostream &os) {

    os << "Usage: p2ecap_standalone [-o output_path] [-d device_name]"
       << " [-m max_packets] [-h]"
       << std::endl;

    os << "Options:"
       << std::endl;

    os << "\t-o, --out         Path to the output directory."
       << std::endl;

    os << "\t-d, --device      Name of the network device to listen on."
       << std::endl;

    os << "\t-m, --max-packets Maximum number of packets to capture."
       << std::endl;

    os << "\t-h, --help        Display this help message."
       << std::endl;

}