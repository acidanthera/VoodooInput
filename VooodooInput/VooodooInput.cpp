#include "VooodooInput.hpp"
#include "VoodooInputMessages.h"
#include "Native Simulator/VoodooInputActuatorDevice.hpp"
#include "Native Simulator/VoodooInputSimulatorDevice.hpp"

#define super IOService
OSDefineMetaClassAndStructors(VoodooInput, IOService);

void VoodooInput::free() {
    OSSafeReleaseNULL(simulator);
    OSSafeReleaseNULL(actuator);
    
    super::free();
}

bool VoodooInput::start(IOService *provider) {
    if (!super::start(provider)) {
        return false;
    }
    
    parentProvider = provider;
    
    OSNumber* transformNumber = OSDynamicCast(OSNumber, provider->getProperty(VOODOO_INPUT_TRANSFORM_KEY));
    
    OSNumber* logicalMaxXNumber = OSDynamicCast(OSNumber, provider->getProperty(VOODOO_INPUT_LOGICAL_MAX_X_KEY));

    OSNumber* logicalMaxYNumber = OSDynamicCast(OSNumber, provider->getProperty(VOODOO_INPUT_LOGICAL_MAX_Y_KEY));

    OSNumber* physicalMaxXNumber = OSDynamicCast(OSNumber, provider->getProperty(VOODOO_INPUT_PHYSICAL_MAX_X_KEY));

    OSNumber* physicalMaxYNumber = OSDynamicCast(OSNumber, provider->getProperty(VOODOO_INPUT_PHYSICAL_MAX_Y_KEY));
    
    if (!transformNumber || !logicalMaxXNumber || !logicalMaxYNumber
            || !physicalMaxXNumber || !physicalMaxYNumber) {
        return false;
    }
    
    transformKey = transformNumber->unsigned8BitValue();
    logicalMaxX = logicalMaxXNumber->unsigned32BitValue();
    logicalMaxY = logicalMaxYNumber->unsigned32BitValue();
    physicalMaxX = physicalMaxXNumber->unsigned32BitValue();
    physicalMaxY = physicalMaxYNumber->unsigned32BitValue();
    
    // Allocate the simulator and actuator devices
    simulator = OSTypeAlloc(VoodooInputSimulatorDevice);
    actuator = OSTypeAlloc(VoodooInputActuatorDevice);
    
    if (!simulator || !actuator) {
        OSSafeReleaseNULL(simulator);
        OSSafeReleaseNULL(actuator);
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
    if (type == kIOMessageVoodooInputMessage && provider == parentProvider) {
        VoodooInputEvent* input = (VoodooInputEvent*)argument;
        
        if (input && simulator) {
            simulator->constructReport(*input);
        }
        
    }
    
    return super::message(type, provider, argument);
}
