//
//  VoodooInputActuatorDevice.cpp
//  VoodooInput
//
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "VoodooInputActuatorDevice.hpp"
#include "VoodooInputIDs.hpp"
#include "VoodooInputMessages.h"

#define super IOHIDDevice
OSDefineMetaClassAndStructors(VoodooInputActuatorDevice, IOHIDDevice);

//const unsigned char actuator_report_descriptor[] = {0x06, 0x00, 0xff, 0x09, 0x0d, 0xa1, 0x01, 0x06, 0x00, 0xff, 0x09, 0x0d, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x85, 0x3f, 0x96, 0x0f, 0x00, 0x81, 0x02, 0x09, 0x0d, 0x85, 0x53, 0x96, 0x3f, 0x00, 0x91, 0x02, 0xc0};
const unsigned char actuator_report_descriptor[] = {
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x85, 0x02, 0x95, 0x03,
    0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05, 0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x02,
    0x81, 0x06, 0xc0, 0xc0
};

IOReturn VoodooInputActuatorDevice::setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) {
    return kIOReturnSuccess;
}

IOReturn VoodooInputActuatorDevice::newReportDescriptor(IOMemoryDescriptor** descriptor) const {
    IOBufferMemoryDescriptor* report_descriptor_buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof(actuator_report_descriptor));
    
    if (!report_descriptor_buffer) {
        IOLog("%s Could not allocate buffer for report descriptor\n", getName());
        return kIOReturnNoResources;
    }
    
    report_descriptor_buffer->writeBytes(0, actuator_report_descriptor, sizeof(actuator_report_descriptor));
    *descriptor = report_descriptor_buffer;
    
    return kIOReturnSuccess;
}

OSString* VoodooInputActuatorDevice::newManufacturerString() const {
    return OSString::withCString("Apple Inc.");
}

OSNumber* VoodooInputActuatorDevice::newPrimaryUsageNumber() const {
    return OSNumber::withNumber(0x2, 32);
}

OSNumber* VoodooInputActuatorDevice::newPrimaryUsagePageNumber() const {
    return OSNumber::withNumber(0x1, 32);
}

OSNumber* VoodooInputActuatorDevice::newProductIDNumber() const {
    return OSNumber::withNumber(VoodooInputGetProductId(), 32);
}

OSString* VoodooInputActuatorDevice::newProductString() const {
    return OSString::withCString("Wellspring Emulation Top Case Buttons");
}

OSString* VoodooInputActuatorDevice::newSerialNumberString() const {
    return OSString::withCString("None");
}

OSString* VoodooInputActuatorDevice::newTransportString() const {
    return OSString::withCString("USB");
}

OSNumber* VoodooInputActuatorDevice::newVendorIDNumber() const {
    return OSNumber::withNumber(kVoodooInputVendorApple, 16);
}

OSNumber* VoodooInputActuatorDevice::newLocationIDNumber() const {
    return OSNumber::withNumber(0x1d183000, 32);
}

OSNumber* VoodooInputActuatorDevice::newVersionNumber() const {
    return OSNumber::withNumber(0x219, 32);
}

IOReturn VoodooInputActuatorDevice::message(UInt32 type, IOService *provider, void *arg) {
    UInt8 btns = (UInt8)(size_t)arg;
    IOLog("%s Message with type %d with %zu\n", getName(), type, (size_t) arg);
    if (type == kIOMessageVoodooInputUpdateBtn) {
        if (btns == buttonState) return kIOReturnSuccess;
        IOBufferMemoryDescriptor *report = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, 4);
        UInt8 *bytes = reinterpret_cast<UInt8 *>(report->getBytesNoCopy());
        bytes[0] = 0x02; // Report ID
        bytes[1] = btns;
        bytes[2] = 0x00; // dx
        bytes[3] = 0x00; // dy
        
        buttonState = btns;
        
        handleReport(report);
        OSSafeReleaseNULL(report);
        return kIOReturnSuccess;
    }
    
    return super::message(type, provider, arg);
}
