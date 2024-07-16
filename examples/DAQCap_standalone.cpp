/**
 * @file p2ecap_standalone.cpp
 *
 * @brief A standalone program that captures miniDAQ data from a network device
 * and writes it to a .dat file.
 * 
 * DAT files are binary files containing raw miniDAQ data with no padding or
 * metadata.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

#include <DAQCap.h>

#include <cstring>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <getopt.h>

using std::vector;
using std::string;

using std::shared_ptr;

using std::cout;
using std::cin;
using std::cerr;
using std::endl;

using DAQCap::Device;
using DAQCap::SessionHandler;

class user_interrupt : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Holds the command-line arguments
struct Arguments {

    // Output directory path
    string outPath;

    // Name of the network device to listen on
    string deviceName;

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
shared_ptr<Device> promptForDevice(
    const vector<shared_ptr<Device>> &devices
);

// Get a timestamp representing the current time in the given format
string getCurrentTimestamp(const string &format);

// Print a list of available network devices
void printDeviceList(
    std::ostream &os, 
    const vector<shared_ptr<Device>> &devices
);

// Print the help message
void printHelp(std::ostream &os);

int main(int argc, char **argv) {

    ///////////////////////////////////////////////////////////////////////////
    // Parse CL arguments and handle help/invalid
    ///////////////////////////////////////////////////////////////////////////

    Arguments args = parseArguments(argc, argv);

    if(!args.valid || args.help) {

        printHelp(cout);

        return 0;

    }

    ///////////////////////////////////////////////////////////////////////////
    // Select a device to listen on
    ///////////////////////////////////////////////////////////////////////////

    SessionHandler handler;

    // Check for the device specified by the user, if applicable
    shared_ptr<Device> device 
        = handler.getNetworkDevice(args.deviceName);

    // If we don't have a device yet, prompt the user for one
    if(!device) {

        if(!args.deviceName.empty()) {

            cout << "No device found with name: " << args.deviceName
                      << endl;

        }

        vector<shared_ptr<Device>> devices;

        try {

            devices = handler.getAllNetworkDevices();

        } catch(const std::exception &e) {

            cerr << e.what() << endl;
            cout << "Exiting..." << endl;

            return 1;

        }

        if(devices.empty()) {

            cout << "No network devices found. Check your permissions."
                      << endl;

        }

        // Print the list of devices
        printDeviceList(cout, devices);

        // Prompt the user to select a device
        try {

            device = promptForDevice(devices);
            
        } catch(const user_interrupt &e) {

            cout << "No device selected. Exiting..." << endl;
            return 0;

        }

    }

    ///////////////////////////////////////////////////////////////////////////
    // Initialize a SessionHandler for the selected device
    ///////////////////////////////////////////////////////////////////////////

    try {

        // Initialize the session handler
        handler.startSession(device);

    } catch(const std::exception &e) {

        cerr << e.what() << endl;
        cout << "Aborted run!" << endl;

        return 1;

    }

    ///////////////////////////////////////////////////////////////////////////
    // Set up output file
    ///////////////////////////////////////////////////////////////////////////

    string runLabel 
        = string("run_") + getCurrentTimestamp("%Y%m%d_%H%M%S");

    string outputFile = args.outPath;

    // If the output directory is nonempty and doesn't end in a slash, add one
    if(!outputFile.empty()) {

        if(outputFile.back() != '/' && outputFile.back() != '\\') {

            outputFile += '/';

        }

    }

    outputFile += runLabel + ".dat";

	std::ofstream fileWriter(outputFile);
	if(!fileWriter.is_open()) {

		cerr << "Failed to open output file: " << outputFile << endl;
        cerr << "Does the output directory exist?" << endl;
		cout << "Aborted run!" << endl;

		return 1;

	}

    cout << "Listening on device: " << device->getName() << endl;
	cout << "Starting run: " << runLabel << endl; 
	cout << "Saving packet data to: " 
              << outputFile 
              << endl;

    ///////////////////////////////////////////////////////////////////////////
    // Fetch packets and write to file
    ///////////////////////////////////////////////////////////////////////////

	int packets   = 0;

	while(true) {

        DAQCap::DataBlob blob;

        try {

            blob = handler.fetchData(std::chrono::minutes(1));

        } catch(const DAQCap::timeout_exception &t) {

            cerr << "Timed out while waiting for packets." << endl;

            continue;

        } catch(const std::exception &e) {

            cerr << e.what() << endl;

            continue; 

        }

        for(const string &warning : blob.warnings()) {

            cerr << warning << endl;

        }

        fileWriter << blob << std::flush;

		packets += blob.packetCount();

        cout << "\rRecorded " << packets << " packets" << std::flush;

        if(packets >= args.maxPackets) {

            break;

        }

	}

    ///////////////////////////////////////////////////////////////////////////
    // Cleanup
    ///////////////////////////////////////////////////////////////////////////

    cout << endl;
	cout << "Data capture finished!" << endl;

    // Look at this for killing threads:
    // https://stackoverflow.com/a/12207835

    return 0;

}

shared_ptr<Device> promptForDevice(
    const vector<shared_ptr<Device>> &devices
) {

    // Prompt user for a device
    int deviceNum = -1;
    string selection;
    do {

        cout << "Select a device (1-" << devices.size() << ")";
        cout << " or select 'q' to quit: ";

        std::getline(cin, selection);

        // Throw an exception if the user wants to quit
        if(selection == "q" || selection == "quit" || selection == "exit") {

            throw user_interrupt("User quit.");

        }

        try {

            deviceNum = std::stoi(selection);

        } catch(std::invalid_argument& e) {

            cout << "Invalid selection:" << selection << endl;
            continue;

        }

        if(deviceNum < 1 || deviceNum > devices.size()) {

            cout << "Invalid selection: " << selection << endl;

        } else {

            break;

        }

    } while(true);

    cout << endl;

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

                    cerr << "-m, --max-packets must take an integer"
                              << " argument." 
                              << endl;

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

string getCurrentTimestamp(const string &format) {

	time_t sys_time;
	struct tm *timeinfo;

	sys_time = time(0);
	timeinfo = localtime(&sys_time);

	char formatBuffer[40];

	std::memset(formatBuffer, 0, sizeof(formatBuffer));
	strftime(formatBuffer, 40, format.data(), timeinfo);

	return string(formatBuffer);

}

void printDeviceList(
    std::ostream &os, 
    const vector<shared_ptr<Device>> &devices
) {

    // Find the biggest name and description so we can format the output
    size_t paddingSize = 4;
    size_t biggestName = 0;
    size_t biggestFullText = 0;
    for(const shared_ptr<Device> device : devices) {

        if(device->getName().size() > biggestName) {
            
            biggestName = device->getName().size();

        }

    }
    for(int i = 0; i < devices.size(); ++i) {

        size_t fullTextSize = biggestName 
            + paddingSize 
            + devices[i]->getDescription().size()
            + (i + 1) / 10 + 3;

        if(fullTextSize > biggestFullText) biggestFullText = fullTextSize;

    }

    // Print the devices
    cout << "Available network devices:" << endl;
    cout << string(
        biggestFullText, 
        '-'
    );
    cout << endl;
    for(int i = 0; i < devices.size(); ++i) {

        string padding(
            biggestName - devices[i]->getName().size() + paddingSize, 
            ' '
        );

        cout << i + 1 
                  << ": " 
                  << devices[i]->getName()
                  << padding 
                  << devices[i]->getDescription() 
                  << endl;

    }
    cout << string(
        biggestFullText, 
        '-'
    );
    cout << endl;

}

void printHelp(std::ostream &os) {

    os << "A standalone program that captures miniDAQ data from a network\n"
       << "device and writes it to a .dat file.\n"
       << endl;

    os << "Usage:" << endl; 
    os << "p2ecap_standalone [-o output_path] [-d device_name]"
       << " [-m max_packets] [-h]\n"
       << endl;

    os << "Options:"
       << endl;

    os << "\t-h, --help        Display this help message."
       << endl;

    os << "\t-o, --out         Path to the output directory."
       << endl;

    os << "\t-d, --device      Name of the network device to listen on."
       << endl;

    os << "\t-m, --max-packets Maximum number of packets to capture. Once\n"
       << "\t                  max-packets are captured, the program will\n"
       << "\t                  finish capturing the current buffer and exit.\n"
       << "\t                  Up to a bufferful of packets past max-packets\n"
       << "\t                  may be captured."
       << endl;

}