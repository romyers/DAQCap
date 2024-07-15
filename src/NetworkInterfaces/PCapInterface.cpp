// Implementation of the NetworkInterface class using the libpcap library.

// Based on work by Liang Guan (guanl@umich.edu).

#include "PCapInterface.h"

#include <stdexcept>
#include <memory>

#include <pcap.h>

using std::unique_ptr;
using std::shared_ptr;

using std::vector;
using std::string;

using namespace DAQCap;

unique_ptr<NetworkManager> NetworkManager::create() {

    return unique_ptr<NetworkManager>(new PCapManager);

}

PCapDevice::PCapDevice(string name, string description):
    name(name), description(description) {}

string PCapDevice::getName() const {

    return name;

}

string PCapDevice::getDescription() const {

    return description;

}

// Global packet buffer we can use to get data out of the fetchPackets() 
// function. It's reallllllly nontrivial to get pcap_dispatch() to read 
// data into a class member, so we have to use this global buffer instead. 
// This is why two threads can't fetch packets from two different devices 
// at the same time. Note that even without this, two threads would not be
// able to fetch packets from the same device at the same time.
// NOTE: The packet buffer is left in a broken state between calls to
//       fetchPackets(). It is the responsibility of the caller to clear the
//       buffer before using it.
vector<Packet> g_packetBuffer;

// Stores the packet data in g_packetBuffer. This function is passed as a 
// callback into pcap_dispatch() during packet fetching.
void listen_callback(
    u_char *useless, 
    const struct pcap_pkthdr *header, 
    const u_char *packet_data
);

// A concrete class we'll be returning as a NetworkManager* from the factory 
// function

PCapManager::~PCapManager() {

    endSession();

}

void PCapManager::endSession() {

    if(handler) pcap_close(handler);
    handler = nullptr;

    g_packetBuffer.clear();

}

bool PCapManager::hasOpenSession() {

    return handler != nullptr;

}

void PCapManager::interrupt() {

    if(handler) pcap_breakloop(handler);

}

void PCapManager::startSession(const shared_ptr<Device> device) {

    if(hasOpenSession()) {

        throw std::logic_error(
            "PCapManager::startSession() cannot be called while another "
            "session is open."
        );

    }

    char errorBuffer[PCAP_ERRBUF_SIZE];

    // Arguments: device name, 
    //            snap length, 
    //            promiscuous mode, 
    //            to_ms, 
    //            error_buffer
    handler = pcap_open_live(
        device->getName().data(), 
        65536, 
        1, 
        10000, 
        errorBuffer
    );
    if(!handler) {

        handler = nullptr;

        throw std::runtime_error(
            string("Could not open device ") 
                + device->getName() + " : " + errorBuffer
        );

    }

    // Compile the filter
    struct bpf_program fcode;
    bpf_u_int32 netmask = 0xffffff;
    char packetFilter[] = "ether src ff:ff:ff:c7:05:01";
    if(pcap_compile(handler, &fcode, packetFilter, 1, netmask) < 0) {

        pcap_freecode(&fcode);
        if(handler) pcap_close(handler);
        handler = nullptr;

        throw std::runtime_error(
            "Unable to compile the packet filter. Check the syntax!"
        );

    }

    if(pcap_setfilter(handler, &fcode) < 0) {

        pcap_freecode(&fcode);
        if(handler) pcap_close(handler);
        handler = nullptr;

        throw std::runtime_error(
            "Filter address error. Cannot apply filter!"
        );

    }

    // pcap_freecode() is used to free up allocated memory pointed to by a
    // bpf_program struct generated by pcap_compile(3PCAP) when that BPF
    // program is no longer needed, for example after it has been made the
    // filter program for a pcap structure by a call to
    // pcap_setfilter(3PCAP).
    pcap_freecode(&fcode);

}

vector<Packet> PCapManager::fetchPackets(int packetsToRead) {

    g_packetBuffer.clear();
    int ret = pcap_dispatch(
        handler, 
        packetsToRead, 
        listen_callback,
        NULL
    );

    if(ret == -1) { // An error occurred

        string errorMessage(pcap_geterr(handler));

        throw std::runtime_error(
            string("Error in pcap_dispatch: ") + errorMessage
        );

    } else if(ret == -2) { // Packet fetching was interrupted

        // NOTE: This is not an exceptional case. We just return an empty
        //       vector of Packets and let the code that called interrupt()
        //       worry about whether it needs special handling.
        return vector<Packet>();

    } 
    
    return std::move(g_packetBuffer);

}

vector<shared_ptr<Device>> PCapManager::getAllDevices() {

    pcap_if_t* deviceHandle = nullptr;
    char errorBuffer[PCAP_ERRBUF_SIZE];

    if(pcap_findalldevs(&deviceHandle, errorBuffer) == -1) {

        if(deviceHandle) pcap_freealldevs(deviceHandle);
        deviceHandle = nullptr;

        throw std::runtime_error(
            string("Error in pcap_findalldevs: ") + errorBuffer
        );

    }

    if(deviceHandle == nullptr) {

        return vector<shared_ptr<Device>>();

    }

    vector<shared_ptr<Device>> devices;

    for(pcap_if_t* d = deviceHandle; d != nullptr; d = d->next) {

        devices.emplace_back(
            new PCapDevice(
                d->name? d->name : "(Unknown Device)",
                d->description? d->description : "(No description available)"
            )
        );

    }

    pcap_freealldevs(deviceHandle);
    deviceHandle = nullptr;

    return devices;

}

void listen_callback(
    u_char *useless, 
    const struct pcap_pkthdr *header, 
    const u_char *packet_data
) {

    try {

        // NOTE: We have to use a structure with global scope here.
        g_packetBuffer.emplace_back(packet_data, header->len);

    } catch(const std::invalid_argument &e) {

        // We don't want to throw exceptions from a callback function.
        // We'll just ignore the malformed packet. Validation code should
        // notice that we missed a packet.
        
    }

    delete[] useless;
    useless = nullptr;

    delete header;
    header = nullptr;

    delete[] packet_data;
    packet_data = nullptr;

}