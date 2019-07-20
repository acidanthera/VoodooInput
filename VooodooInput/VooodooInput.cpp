#include "VooodooInput.hpp"

#define super IOService


bool VoodooInput::init(OSDictionary *properties) {
    if (!super::init(properties))
        return false;
    
    simulator = OSTypeAlloc(VoodooI2CMT2SimulatorDevice);
    actuator = OSTypeAlloc(VoodooI2CMT2ActuatorDevice);
    
    if (!simulator || !actuator) {
        OSSafeReleaseNULL(simulator);
        OSSafeReleaseNULL(actuator);
        return false;
    }
    
    return true;
}

void VoodooInput::free() {
    OSSafeReleaseNULL(simulator);
    OSSafeReleaseNULL(actuator);
    
    super::free();
}

bool VoodooInput::start(IOService *provider) {
    if (!super::start(provider)) {
        return false;
    }
    
    // Initialize simulator device
    if (!simulator->init(NULL) || !simulator->attach(this))
        goto exit;
    else if (!simulator->start(this)) {
        simulator->detach(this);
        goto exit;
    }
    
    // Initialize actuator device
    if (!actuator->init(NULL) || !actuator->attach(this))
        goto exit;
    else if (!actuator->start(this)) {
        actuator->detach(this);
        goto exit;
    }
    
    return true;

exit:
    return false;
}

void VoodooInput::stop(IOService *device) {
    super::stop(device);
}

IOReturn VoodooInput::message(UInt32 type, IOService *provider, void *argument) {
    IOReturn retValue = super::message(type, provider, argument);
    
    return retValue;
}
