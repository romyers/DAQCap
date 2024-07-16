\page Developer Notes

# Developer Notes

TODO: Discuss:
  Packet
  NetworkManager
  Device
  DAQCap.cpp

TODO: Go over doxygen documentation and make sure it's ready to push
TODO: Make sure we have the right github repo for the fetchContent docs

TODO: Note to developers what part of the code they need to work with to
do what they want to do. E.g. new data formats mean working with Packet, 
different network interfaces mean working with NetworkInterface
  - This goes in a 'Developer Notes' markdown that is only included if
    internal documentation is built.
  - Adding new network interfaces
      - Add a finder to cmake
      - Set the NETWORK_INTERFACE cmake variable
      - Link and set a compiler definition for the new network interface
        if NETWORK_INTERFACE is set to the new network interface
      - #elifdef the network interface compiler definition in 
        NetworkInterface.cpp
      - Implement NetworkInterface.h inside the #elifdef
      - Implement NetworkInterface::Create() inside the #elifdef
      - Work will need to be done to make sure that necessary info gets from
        getDeviceList() to Listener::create(). Probably rewrite Device to be
        an abstract base class.