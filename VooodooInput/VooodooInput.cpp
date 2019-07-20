#include "VooodooInput.hpp"
#include "Native Simulator/VoodooActuatorDevice.hpp"
#include "Native Simulator/VoodooSimulatorDevice.hpp"

#define super IOService
OSDefineMetaClassAndStructors(VoodooInput, IOService);

bool VoodooInput::init(OSDictionary *properties) {
    if (!super::init(properties))
        return false;
    
    simulator = OSTypeAlloc(VoodooSimulatorDevice);
    actuator = OSTypeAlloc(VoodooActuatorDevice);
    
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

void VoodooInput::stop(IOService *provider) {
    if (simulator) {
        simulator->stop(this);
        simulator->detach(this);
    }
    
    if (actuator) {
        actuator->stop(this);
        actuator->detach(this);
    }
    
    super::stop(provider);
}

UInt8 VoodooInput::getTransformKey() {
    return transformKey;
}

UInt32 VoodooInput::getPhysicalMaxX() {
    return physicalMaxX;
}

UInt32 VoodooInput::getPhysicalMaxY() {
    return physicalMaxY;
}

UInt32 VoodooInput::getLogicalMaxX() {
    return logicalMaxX;
}

UInt32 VoodooInput::getLogicalMaxY() {
    return logicalMaxY;
}

IOReturn VoodooInput::message(UInt32 type, IOService *provider, void *argument) {
    IOReturn retValue = super::message(type, provider, argument);
    
    return retValue;
}
