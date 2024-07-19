#include <DAQCap.h>

#include "Packet.h"
#include "PacketProcessor.h"

#include <pcap.h>

#include <stdexcept>
#include <future>
#include <map>

using std::string;
using std::vector;
using std::map;

using namespace DAQCap;

class PCapDevice;

// TODO: Look for more ways to split this up. Maybe wrap PCap-specific stuff.

// Global packet buffer we can use to get data out of the fetchData() 
// function. It's reallllllly nontrivial to get pcap_dispatch() to read 
// data into a class member, so we have to use this global buffer instead. 
// This is why two threads can't fetch packets from two different devices 
// at the same time. Note that even without this, two threads would not be
// able to fetch packets from the same device at the same time.
// NOTE: The packet buffer is left in a broken state between calls to
//       fetchData(). It is the responsibility of the caller to clear the
//       buffer before using it.
vector<Packet> g_packetBuffer;

// Global map of existing devices we can use to keep device instances unique.
map<string, PCapDevice> g_devices;

// Stores the packet data in g_packetBuffer. This function is passed as a 
// callback into pcap_dispatch() during packet fetching.
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

class PCapDevice final : public Device {

public:

    PCapDevice(std::string name, std::string description);
    virtual ~PCapDevice();

    virtual std::string getName() const override;
    virtual std::string getDescription() const override;

    virtual void open() override;
    virtual void close() override;

    virtual void interrupt() override;

    virtual DataBlob fetchData(
        std::chrono::milliseconds timeout = FOREVER,
        int packetsToRead = ALL_PACKETS
    ) override;

    PCapDevice(PCapDevice &other) = delete;
    PCapDevice& operator=(PCapDevice &other) = delete;

private:

    std::string name;
    std::string description;

    PacketProcessor packetProcessor;

    pcap_t *handler;

};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

string PCapDevice::getName() const {

    return name;

}

string PCapDevice::getDescription() const {

    return description;

}

vector<Device*> Device::getAllDevices() {

    ///////////////////////////////////////////////////////////////////////////
    // Get the devices from pcap
    ///////////////////////////////////////////////////////////////////////////

    // OPTIMIZATION -- If fetching pcap devices is slow, we can cache them

    pcap_if_t* deviceHandle = nullptr;
    char errorBuffer[PCAP_ERRBUF_SIZE];

    if(pcap_findalldevs(&deviceHandle, errorBuffer) == -1) {

        if(deviceHandle) pcap_freealldevs(deviceHandle);
        deviceHandle = nullptr;

        throw std::runtime_error(
            string("Error in pcap_findalldevs: ") + errorBuffer
        );

    }

    ///////////////////////////////////////////////////////////////////////////
    // Find/create the devices in the cache and return pointers
    ///////////////////////////////////////////////////////////////////////////

    vector<Device*> devices;

    if(deviceHandle == nullptr) {

        return devices;

    }

    for(pcap_if_t* d = deviceHandle; d != nullptr; d = d->next) {

        if(g_devices.find(d->name) == g_devices.end()) {

            // Getting g_devices to emplace without copying is a little ugly
            g_devices.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(d->name),
                std::forward_as_tuple(
                    d->name       ? d->name        : "(Unknown Device)",
                    d->description? d->description : "(No description available)"
                )
            );

        }

        devices.push_back(&g_devices.at(d->name));

    }

    pcap_freealldevs(deviceHandle);
    deviceHandle = nullptr;

    return devices;

}

Device *Device::getDevice(const string &name) {

    vector<Device*> devices = getAllDevices();

    for(Device *device : devices) {

        if(device->getName() == name) return device;

    }

    return nullptr;

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

PCapDevice::PCapDevice(std::string name, std::string description)
    : name(name), description(description), handler(nullptr) {}

void PCapDevice::open() {

    // Don't do anything if the device is already open
    if(handler) return;

    char errorBuffer[PCAP_ERRBUF_SIZE];

    handler = pcap_create(getName().data(), errorBuffer);
    if(!handler) {

        handler = nullptr;

        throw std::runtime_error(
            string("Could not open device ") 
                + getName() + " : " + errorBuffer
        );

    }

    pcap_set_snaplen(handler, 65536);
    pcap_set_promisc(handler, 1);

    // OPTIMIZATION: If this is too slow, turn off immediate mode and control
    //               the buffer size to get a good balance between speed and
    //               responsiveness.
    
    // With immediate_mode on, packets are delivered to the application as soon
    // as they are received. With immediate_mode off, packets are buffered 
    // until the buffer is full or a timeout occurs.
    pcap_set_immediate_mode(handler, 1);
    // pcap_set_timeout(handler, 10000);   // This does not work on every OS
    // pcap_set_buffer_size(handler, 100); // How many bytes to buffer before
    //                                     // delivering packets (not used in
    //                                     // immediate mode)

    // TODO: With immediate_mode, is there really a reason to use pcap_dispatch
    //       instead of just getting packets one at a time?

    int ret = pcap_activate(handler);
    if(ret < 0) {

        throw std::runtime_error(
            string("Could not activate device ") 
                + getName() + " : " + pcap_geterr(handler)
                + ".\nCheck permissions."
        );

        pcap_close(handler);
        handler = nullptr;

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

void PCapDevice::close() {

    interrupt();

    if(handler) pcap_close(handler);
    handler = nullptr;

    g_packetBuffer.clear();
    packetProcessor.reset();

}

void PCapDevice::interrupt() {

    if(!handler) return;

    // NOTE: This does not unblock pcap_dispatch() for versions of
    //       libpcap earlier than 1.10.0 or for systems other than
    //       Linux or Windows. We need to use a different interrupt
    //       solution in these cases.

    // These OSes allow pcap_breakloop to wake up the pcap_dispatch thread
    #if defined(__linux__) || defined(_WIN32) || defined(_WIN64)

        // I *think* this was first introduced in 1.10.0, so it should work
        // as a way to check that we are running 1.10.0 or later.
        #ifdef PCAP_CHAR_ENC_LOCAL

            pcap_breakloop(handler);
            return;

        #endif

    #endif

    // If we get this far, then we can't interrupt


    // TODO: Document that we can't interrupt on this platform or
    //       find another way to do it. We could consider e.g.
    //       sending a special packet and catching that in the
    //       listen function. We have to make sure that sending
    //       the packet won't interfere with the miniDAQ though.

}

DataBlob PCapDevice::fetchData(
    std::chrono::milliseconds timeout,
    int packetsToRead
) {

    // TODO: Timeout logic for versions that can't interrupt

    if(!handler) {

        throw std::logic_error(
            "Data cannot be fetched without an open session."
        );

    }

    DataBlob blob;

    ///////////////////////////////////////////////////////////////////////////
    // Run a thread that listens for packets
    ///////////////////////////////////////////////////////////////////////////

    auto future = std::async(
        std::launch::async, 
        [this, packetsToRead]() {

            return pcap_dispatch(
                handler, 
                packetsToRead, 
                listen_callback,
                NULL
            );

        }
    );

    ///////////////////////////////////////////////////////////////////////////
    // Throw if we have to wait too long for the thread to complete
    ///////////////////////////////////////////////////////////////////////////

    if(
        timeout != FOREVER &&
        future.wait_for(
            timeout
        ) == std::future_status::timeout
    ) {

        // This should force the netManager to return early
        interrupt();

        throw timeout_exception(
            "DAQCap::Device::fetchData() timed out"
        );

        // NOTE: Future will block until completion when it goes out of scope,
        //       so we don't need to explicitly wait for it

    }

    ///////////////////////////////////////////////////////////////////////////
    // Get the packet data and return it as a processed blob
    ///////////////////////////////////////////////////////////////////////////

    int ret = future.get();

    vector<Packet> packets;

    // Swap g_packetBuffer with the local vector, clearing g_packetBuffer in
    // the process
    std::swap(packets, g_packetBuffer);

    if(ret == -1) { // An error occurred

        string errorMessage(pcap_geterr(handler));

        throw std::runtime_error(
            string("Could not fetch data: ") + errorMessage
        );

    } else if(ret == -2) { // Packet fetching was interrupted

        // NOTE: This is not an exceptional case. We just return an empty
        //       vector of Packets and let the code that called interrupt()
        //       worry about whether it needs special handling.

    } 
    
    // TODO: Is this faster with std::move?
    return packetProcessor.process(packets);

}

PCapDevice::~PCapDevice() {

    close();

}