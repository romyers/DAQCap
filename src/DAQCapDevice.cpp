#include <DAQCapDevice.h>

using namespace DAQCap;

Device::Device() {}

Device::Device(std::string name, std::string description):
    name(name), description(description) {}

std::string Device::getName() const {

    return name;

}

std::string Device::getDescription() const {

    return description;

}

Device::operator bool() const {
    
    return !name.empty();

}