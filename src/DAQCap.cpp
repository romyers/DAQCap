#include <DAQCap.h>

#include "Packet.h"
#include "PacketProcessor.h"

#include <pcap.h>

#include <stdexcept>
#include <map>

using std::string;
using std::vector;
using std::map;

using namespace DAQCap;

// Let's make a preprocessor macro to tell us if interrupts are supported.
// These OSes allow pcap_breakloop to wake up the pcap_dispatch thread
#if defined(__linux__) || defined(_WIN32) || defined(_WIN64)

	// I *think* this was first introduced in 1.10.0, so it should work
	// as a way to check that we are running 1.10.0 or later.
	#ifdef PCAP_CHAR_ENC_LOCAL

		#define INTERRUPT_SUPPORTED

	#endif

#endif

bool DAQCap::interrupt_supported() {

	#ifdef INTERRUPT_SUPPORTED

		return true;

	#else

		return false;

	#endif

}

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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class PCapDevice final : public Device {

public:

	PCapDevice(std::string name, std::string description);
	virtual ~PCapDevice();

	virtual std::string getName() const override;
	virtual std::string getDescription() const override;

	virtual void open() override;
	virtual bool is_open() const override;
	virtual void close() override;

	virtual void interrupt() override;

	virtual DataBlob fetchData(
		std::chrono::seconds timeout = FOREVER,
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

	vector<Device*> devices;

	///////////////////////////////////////////////////////////////////////////
	// Get the devices from pcap
	///////////////////////////////////////////////////////////////////////////

	// OPTIMIZATION -- If fetching pcap devices is slow, we can cache them

	pcap_if_t* deviceHandle = nullptr;
	char errorBuffer[PCAP_ERRBUF_SIZE];

	if(pcap_findalldevs(&deviceHandle, errorBuffer) == -1) {

		if(deviceHandle) pcap_freealldevs(deviceHandle);
		deviceHandle = nullptr;

		return devices;

	}

	if(deviceHandle == nullptr) {

		return devices;

	}

	///////////////////////////////////////////////////////////////////////////
	// Find/create the devices in the cache and return pointers
	///////////////////////////////////////////////////////////////////////////

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

		return;

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

	pcap_set_timeout(handler, 10000);   // This does not work on every OS
	pcap_set_buffer_size(handler, 100); // How many bytes to buffer before
										// delivering packets (not used in
										// immediate mode)

	// TODO: In immediate_mode, is there really a reason to use pcap_dispatch
	//       instead of just getting packets one at a time?

	int ret = pcap_activate(handler);
	if(ret < 0) {

		if(handler) pcap_close(handler);
		handler = nullptr;

		return;

	}

	// Compile the filter
	struct bpf_program fcode;
	bpf_u_int32 netmask = 0xffffff;
	char packetFilter[] = "ether src ff:ff:ff:c7:05:01";
	if(pcap_compile(handler, &fcode, packetFilter, 1, netmask) < 0) {

		pcap_freecode(&fcode);
		if(handler) pcap_close(handler);
		handler = nullptr;

		return;

	}

	if(pcap_setfilter(handler, &fcode) < 0) {

		pcap_freecode(&fcode);
		if(handler) pcap_close(handler);
		handler = nullptr;

		return;

	}

	// pcap_freecode() is used to free up allocated memory pointed to by a
	// bpf_program struct generated by pcap_compile(3PCAP) when that BPF
	// program is no longer needed, for example after it has been made the
	// filter program for a pcap structure by a call to
	// pcap_setfilter(3PCAP).
	pcap_freecode(&fcode);

	// TODO: Idea -- Start buffering packets immediately when open() is called,
	//       and let the fetch function just read out the buffer.

}

bool PCapDevice::is_open() const {

	return handler != nullptr;

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
	if(interrupt_supported()) {

		pcap_breakloop(handler);
		return;

	}

	// If we get this far, then we can't interrupt

	// TODO: Document that we can't interrupt on this platform or
	//       find another way to do it. We could consider e.g.
	//       sending a special packet and catching that in the
	//       listen function. We have to make sure that sending
	//       the packet won't interfere with the miniDAQ though.

}

DataBlob PCapDevice::fetchData(
	std::chrono::seconds timeout,
	int packetsToRead
) {

	// TODO: It would be great if fetchData could abandon the dispatch thread
	//       and return on interrupt. We can make sure the dispatch thread will
	//       end *eventually* and just detach it.

	DataBlob blob;

	// TODO: Timeout logic for versions that can't interrupt

	if(!handler) {

		throw std::runtime_error(
			"The device is not open."
		);

	}

	///////////////////////////////////////////////////////////////////////////
	// Read the data with timeout logic
	///////////////////////////////////////////////////////////////////////////

	// An earlier implementation waited for an std::future produced by
	// std::async. However, this implementation only worked when interrupts
	// were supported. The following OS-specific
	// implementations will work for older versions of libpcap.

	// If the timeout logic leaves sel > 0, then we can call pcap_dispatch().
	// Otherwise, something went wrong or we timed out.
	int sel = 1;
	if(timeout != FOREVER) {

		#if defined(__linux__) || defined(__APPLE__)

			// TODO: Verify that this works on MacOS

			fd_set rfds;        // file descriptor sets for "select" function
								// (it's a bit arrray)
			struct timeval tv;  // strcuture represents elapsed time (declared
								// in sys/time.h)

			// Get a file descriptor for the pcap device
			int fd = pcap_get_selectable_fd(handler); 

			FD_ZERO(&rfds); //re-clears(empty) file descriptor set 
			FD_SET(fd,&rfds); //rebuild file descriptor set
			
			tv.tv_sec=timeout.count();
			tv.tv_usec=0;
		
			// TODO: Linux docs say that poll() is preferred to select() for
			//       modern applications
			// Blocks the calling process until there is activity on the file
			// descriptor or the timeout period has expired
			sel = select(fd, &rfds, NULL, NULL, &tv); 


		#elif defined(_WIN32) || defined(_WIN64)

			// TODO: Windows support

		#else

			// There's nothing we can do, so just skip the timeout
			// entirely

		#endif

	}

	// Now we're done with the timeout logic, so we can get the data
	int ret = -1;
	if(sel > 0) {

		ret = pcap_dispatch(
			handler, 
			packetsToRead, 
			listen_callback,
			NULL
		);

	} else if(sel == 0) {

		// We timed out
		ret = -2;

	} else {

		// select() failed
		ret = -1;

	}

	///////////////////////////////////////////////////////////////////////////
	// Get data from global buffer
	///////////////////////////////////////////////////////////////////////////

	// Swap g_packetBuffer with the local vector, clearing g_packetBuffer in
	// the process
	vector<Packet> packets;
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

	///////////////////////////////////////////////////////////////////////////
	// Blobify and return data
	///////////////////////////////////////////////////////////////////////////
	
	// TODO: Is this faster with std::move?
	return packetProcessor.blobify(packets);

}

PCapDevice::~PCapDevice() {

	close();

}