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

PCapDevice::PCapDevice(string name, string description):
    name(name), description(description) {}

string PCapDevice::getName() const {

    return name;

}

string PCapDevice::getDescription() const {

    return description;

}
PCapManager::PCapManager() {
    
    // Some older versions of libpcap don't support this init function. We can
    // catch that by checking if PCAP_CHAR_ENC_LOCAL is defined.
    #if defined(PCAP_CHAR_ENC_LOCAL)

        static bool initialized = false;

        // NOTE: pcap_init doesn't work for older versions of libpcap
        if(!initialized) {

            char errorbuf[PCAP_ERRBUF_SIZE];

            initialized = true;
            pcap_init(PCAP_CHAR_ENC_LOCAL, errorbuf);

        }

    #endif

}

PCapManager::~PCapManager() {

    endSession();

}

void PCapManager::endSession() {

    if(handler) pcap_close(handler);
    handler = nullptr;

    g_packetBuffer.clear();

}

void PCapManager::interrupt() {

    // NOTE: This does not unblock pcap_dispatch() for versions of
    //       libpcap earlier than 1.10.0 or for systems other than
    //       Linux or Windows. We need to use a different interrupt
    //       solution in these cases.

    if(handler) {

        // These OSes allow pcap_breakloop to wake up the pcap_dispatch thread
        #if defined(__linux__) || defined(_WIN32) || defined(_WIN64)

            // I *think* this was first introduced in 1.10.0, so it should work
            // as a way to check that we are running 1.10.0 or later.
            #ifdef PCAP_CHAR_ENC_LOCAL

                pcap_breakloop(handler);

            #endif

        #endif

        // TODO: Document that we can't interrupt on this platform or
        //       find another way to do it. We could consider e.g.
        //       sending a special packet and catching that in the
        //       listen function. We have to make sure that sending
        //       the packet won't interfere with the miniDAQ though.

    }

}

void PCapManager::startSession(const shared_ptr<Device> device) {

    if(handler) {

        throw std::logic_error(
            "Cannot start a new session while another session is in progress."
        );

    }

    char errorBuffer[PCAP_ERRBUF_SIZE];

    handler = pcap_create(device->getName().data(), errorBuffer);
    if(!handler) {

        handler = nullptr;

        throw std::runtime_error(
            string("Could not open device ") 
                + device->getName() + " : " + errorBuffer
        );

    }

    pcap_set_snaplen(handler, 65536);
    pcap_set_promisc(handler, 1);
    
    // With immediate_mode on, packets are delivered to the application as soon
    // as they are received. With immediate_mode off, packets are buffered 
    // until the buffer is full or a timeout occurs.
    pcap_set_immediate_mode(handler, 1);
    pcap_set_timeout(handler, 10000); // This does not work on every OS

    // TODO: With immediate_mode, is there really a reason to use pcap_dispatch
    //       instead of just getting packets one at a time?

    pcap_activate(handler);

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

    if(!handler) {

        throw std::logic_error(
            "Data cannot be fetched without an open session."
        );

    }

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
    
    // This will leave g_packetBuffer in an unspecified (but valid) state. 
    // We need to clear it before we can use it again.
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
                d->name       ? d->name        : "(Unknown Device)",
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

    } catch(...) {

        // We don't want to throw exceptions from a callback function.
        // We'll just ignore the malformed packet. DAQCap::SessionHandler will
        // notice that we missed a packet.
        
    }

}