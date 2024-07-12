

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

# TODO: Does NetworkInterface actually need to be abstract if the interface
#       implementation is set at compile time?
#         - The abstract class is only useful if the interface is set at
#           runtime. That can be done -- see:
#           https://stackoverflow.com/a/9039693
#         - Actually, nvm. We need to hide private data members from the
#           interface, which means we either PIMPL or use an abstract
#           base class.