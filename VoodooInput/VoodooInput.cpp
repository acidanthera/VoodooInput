#include "VoodooInput.hpp"
#include "VoodooInputMessages.h"
#include "VoodooInputSimulator/VoodooInputActuatorDevice.hpp"
#include "VoodooInputSimulator/VoodooInputSimulatorDevice.hpp"

#define super IOService
OSDefineMetaClassAndStructors(VoodooInput, IOService);

void VoodooInput::free() {
    OSSafeReleaseNULL(simulator);
    OSSafeReleaseNULL(actuator);
    
    super::free();
}

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
    
    if (!transformNumber || !logicalMaxXNumber || !logicalMaxYNumber
            || !physicalMaxXNumber || !physicalMaxYNumber) {
        IOLog("Kishor VoodooInput could not get provider properties!\n");
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
        IOLog("Kishor VoodooInput could not alloc simulator or actuator!\n");
        OSSafeReleaseNULL(simulator);
        OSSafeReleaseNULL(actuator);
        return false;
    }
    
    // Initialize simulator device
    if (!simulator->init(NULL) || !simulator->attach(this)) {
        IOLog("Kishor VoodooInput could not attach simulator!\n");
        goto exit;
    }
    else if (!simulator->start(this)) {
        IOLog("Kishor VoodooInput could not start simulator!\n");
        simulator->detach(this);
        goto exit;
    }
    
    // Initialize actuator device
    if (!actuator->init(NULL) || !actuator->attach(this)) {
        IOLog("Kishor VoodooInput could not init or attach actuator!\n");
        goto exit;
    }
    else if (!actuator->start(this)) {
        IOLog("Kishor VoodooInput could not start actuator!\n");
        actuator->detach(this);
        goto exit;
    }
    
    setProperty(VOODOO_INPUT_IDENTIFIER, kOSBooleanTrue);
    
    if (!parentProvider->open(this)) {
        IOLog("Kishor VoodooInput could not open!\n");
        return false;
    };
    
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
    
    if (parentProvider->isOpen(this)) {
        parentProvider->close(this);
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

#ifdef __MAC_10_15

/**
 *  Ensure the symbol is not exported
 */
#define PRIVATE __attribute__((visibility("hidden")))

/**
 *  For private fallback symbol definition
 */
#define WEAKFUNC __attribute__((weak))

// macOS 10.15 adds Dispatch function to all OSObject instances and basically
// every header is now incompatible with 10.14 and earlier.
// Here we add a stub to permit older macOS versions to link.
// Note, this is done in both kern_util and plugin_start as plugins will not link
// to Lilu weak exports from vtable.

kern_return_t WEAKFUNC PRIVATE OSObject::Dispatch(const IORPC rpc) {
    (panic)("OSObject::Dispatch plugin stub called");
}

kern_return_t WEAKFUNC PRIVATE OSMetaClassBase::Dispatch(const IORPC rpc) {
    (panic)("OSMetaClassBase::Dispatch plugin stub called");
}

#endif
