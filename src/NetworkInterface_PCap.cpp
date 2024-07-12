// Implementation of the NetworkInterface class using the libpcap library.

// Based on work by Liang Guan (guanl@umich.edu).

#include "NetworkInterface.h"

#include <stdexcept>

#include <pcap.h>

using namespace DAQCap;

// Global packet buffer we can use to get data out of the listen() function.
// It's reallllllly nontrivial to get pcap_dispatch() to read data into a
// class member, so we have to use this global buffer instead. This is why
// two threads can't fetch packets from two different devices at the same time.
// Note that even without this, two threads would not be able to fetch packets
// from the same device at the same time.
std::vector<Packet> g_packetBuffer;

// Stores the packet data in g_packetBuffer. This function is passed as a 
// callback into pcap_dispatch() during packet fetching.
void listen_callback(
	u_char *useless, 
	const struct pcap_pkthdr *header, 
	const u_char *packet_data
);

// TODO: For testing, write an implementation that reads a pcap save file:
//       https://www.tcpdump.org/manpages/pcap_open_offline.3pcap.html

// A concrete class we'll be returning as a Listener* from the factory 
// function
class PCap_Listener: public Listener {

public:

    PCap_Listener(const Device &device);
    ~PCap_Listener();

    PCap_Listener(const PCap_Listener &other)            = delete;
    PCap_Listener &operator=(const PCap_Listener &other) = delete;

    void interrupt() override;
    std::vector<Packet> listen(int packetsToRead) override;

private:

    pcap_t *handler = nullptr;

};

PCap_Listener::PCap_Listener(const Device &device) {

	char errorBuffer[PCAP_ERRBUF_SIZE];

    // Arguments: device name, snap length, promiscuous mode, to_ms, error_buffer
	handler = pcap_open_live(device.name.data(), 65536, 1, 10000, errorBuffer);
	if(!handler) {

		handler = nullptr;

		throw std::runtime_error(
			std::string("Could not open device ") 
                + device.name + " : " + errorBuffer
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

PCap_Listener::~PCap_Listener() {

	if(handler) pcap_close(handler);
	handler = nullptr;

}

void PCap_Listener::interrupt() {

    if(handler) pcap_breakloop(handler);

}

std::vector<Packet> PCap_Listener::listen(int packetsToRead) {

    // TODO: We could lock a mutex while we're messing with g_packetBuffer
    //       and pcap_dispatch to make this thread-safe. I kinda prefer to keep
    //       synchronization at the level where the threads are being created
    //       though, in which case we can just let client code provide
    //       its own synchronization if it needs it.

    g_packetBuffer.clear();
    int ret = pcap_dispatch(
        handler, 
        packetsToRead, 
        listen_callback,
        NULL
    );

    if(ret == -1) { // An error occurred

        std::string errorMessage(pcap_geterr(handler));

        throw std::runtime_error(
            std::string("Error in pcap_dispatch: ") + errorMessage
        );

    } else if(ret == -2) { // Packet fetching was interrupted

        // NOTE: This is not an exceptional case. We just return an empty
        //       vector of Packets and let the code that called interrupt()
        //       worry about whether it needs special handling.
        return std::vector<Packet>();

    } 
    
    return std::move(g_packetBuffer);

}

std::vector<Device> DAQCap::getDevices() {

    // TODO: Weed out inactive devices?

    pcap_if_t* deviceHandle = nullptr;
    char errorBuffer[PCAP_ERRBUF_SIZE];

    if(pcap_findalldevs(&deviceHandle, errorBuffer) == -1) {

        if(deviceHandle) pcap_freealldevs(deviceHandle);
        deviceHandle = nullptr;

        throw std::runtime_error(
            std::string("Error in pcap_findalldevs: ") + errorBuffer
        );

    }

    if(deviceHandle == nullptr) {

        throw std::runtime_error(
            "No network devices found. Check your permissions."
        );

    }

    std::vector<Device> devices;

    for(pcap_if_t* d = deviceHandle; d != nullptr; d = d->next) {

        Device device;

        device.name = d->name? 
            d->name : "(Unknown Device)";

        device.description = d->description? 
            d->description : "(No description available)";

        devices.push_back(device);

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

Listener *Listener::create(const Device &device) {

    return new PCap_Listener(device);

}