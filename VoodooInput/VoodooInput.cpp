//
//  VoodooInput.сpp
//  VoodooInput
//
//  Copyright © 2019 Kishor Prins. All rights reserved.
//

#include "VoodooInput.hpp"
#include "VoodooInputMultitouch/VoodooInputMessages.h"
#include "VoodooInputSimulator/VoodooInputActuatorDevice.hpp"
#include "VoodooInputSimulator/VoodooInputSimulatorDevice.hpp"

#define super IOService
OSDefineMetaClassAndStructors(VoodooInput, IOService);

bool VoodooInput::start(IOService *provider) {
    if (!super::start(provider)) {
        IOLog("Kishor VoodooInput could not super::start!\n");
        return false;
    }
    
    parentProvider = provider;
    
    OSNumber* transformNumber = OSDynamicCast(OSNumber, provider->getProperty(VOODOO_INPUT_TRANSFORM_KEY));
    
    OSNumber* logicalMaxXNumber = OSDynamicCast(OSNumber, provider->getProperty(VOODOO_INPUT_LOGICAL_MAX_X_KEY));

    OSNumber* logicalMaxYNumber = OSDynamicCast(OSNumber, provider->getProperty(VOODOO_INPUT_LOGICAL_MAX_Y_KEY));

    OSNumber* physicalMaxXNumber = OSDynamicCast(OSNumber, provider->getProperty(VOODOO_INPUT_PHYSICAL_MAX_X_KEY));

    OSNumber* physicalMaxYNumber = OSDynamicCast(OSNumber, provider->getProperty(VOODOO_INPUT_PHYSICAL_MAX_Y_KEY));
    
    if (transformNumber == nullptr || logicalMaxXNumber == nullptr || logicalMaxYNumber == nullptr ||
		physicalMaxXNumber == nullptr || physicalMaxYNumber == nullptr) {
        IOLog("VoodooInput could not get provider properties!\n");
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
        IOLog("VoodooInput could not alloc simulator or actuator!\n");
        OSSafeReleaseNULL(simulator);
        OSSafeReleaseNULL(actuator);
        return false;
    }
    
    // Initialize simulator device
    if (!simulator->init(NULL) || !simulator->attach(this)) {
        IOLog("VoodooInput could not attach simulator!\n");
        goto exit;
    }
    else if (!simulator->start(this)) {
        IOLog("VoodooInput could not start simulator!\n");
        simulator->detach(this);
        goto exit;
    }
    
    // Initialize actuator device
    if (!actuator->init(NULL) || !actuator->attach(this)) {
        IOLog("VoodooInput could not init or attach actuator!\n");
        goto exit;
    }
    else if (!actuator->start(this)) {
        IOLog("VoodooInput could not start actuator!\n");
        actuator->detach(this);
        goto exit;
    }
    
    setProperty(VOODOO_INPUT_IDENTIFIER, kOSBooleanTrue);
    
    if (!parentProvider->open(this)) {
        IOLog("VoodooInput could not open!\n");
        return false;
    };
    
    return true;

exit:
    return false;
}

bool VoodooInput::willTerminate(IOService* provider, IOOptionBits options) {
    if (parentProvider->isOpen(this)) {
        parentProvider->close(this);
    }

    return super::willTerminate(provider, options);
}

void VoodooInput::stop(IOService *provider) {
    if (simulator) {
        simulator->stop(this);
        simulator->detach(this);
        OSSafeReleaseNULL(simulator);
    }
    
    if (actuator) {
        actuator->stop(this);
        actuator->detach(this);
        OSSafeReleaseNULL(actuator);
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
        if (argument && simulator) {
            simulator->constructReport(*(VoodooInputEvent*)argument);
        }
    }
    else if (type == kIOMessageVoodooInputUpdateDimensionsMessage && provider == parentProvider) {
        if (argument) {
            const VoodooInputDimensions& dimensions = *(VoodooInputDimensions*)argument;
            logicalMaxX = dimensions.max_x - dimensions.min_x;
            logicalMaxY = dimensions.max_y - dimensions.min_y;
        }
    }

    return super::message(type, provider, argument);
}
