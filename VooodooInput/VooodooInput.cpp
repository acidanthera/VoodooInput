#include "VooodooInput.hpp"

#define super IOService

bool VoodooInput::start(IOService *provider) {
    if (!super::start(provider)) {
        return false;
    }
    
    return true;
}

void VoodooInput::stop(IOService *device) {
    super::stop(device);
}

IOReturn VoodooInput::message(UInt32 type, IOService *provider, void *argument) {
    IOReturn retValue = super::message(type, provider, argument);
    
    return retValue;
}
