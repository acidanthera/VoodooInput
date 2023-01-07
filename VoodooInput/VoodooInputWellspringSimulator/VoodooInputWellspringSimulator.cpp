//
//  AppleUSBMultitouchDriver.cpp
//  VoodooInput
//
//  Created by Avery Black on 12/31/22.
//  Copyright © 2022 Kishor Prins. All rights reserved.
//

#include "VoodooInputWellspringSimulator.hpp"
#include "VoodooInputWellspringUserClient.hpp"
#include "VoodooInputActuatorDevice.hpp"
#include <libkern/version.h>
#include "VoodooInputMessages.h"

#define super IOHIDDevice
OSDefineMetaClassAndStructors(VoodooInputWellspringSimulator, IOHIDDevice);

// Reports come from a MacbookPro9,2
#define SensorParamsLength 6
static const UInt8 MTSensorParams[SensorParamsLength] = {0x00, 0x00, 0x03, 0x00, 0xD6, 0x01};

#define SensorDescLength 8
static const UInt8 MTSensorDesc[SensorDescLength] = {0x01, 0x01, 0x00, 0x0c, 0x01, 0x00, 0x14, 0x00};

#define SensorRowsLength 5
static const UInt8 MTSensorRows[SensorRowsLength] = {
    0x01,       // Is little Endian
    0x0C,       // Rows
    0x14,       // Columns
    0x01, 0x09  // BCD Version
};

#define FamilyID 0x62
#define FamilyIDLength 1

bool VoodooInputWellspringSimulator::setProperty(const char *aKey, OSObject *anObject) {
    IOLog("MT1Sim: Attempting to set Property! %s\n", aKey);
    OSString *str = OSString::withCString(aKey);
    bool ret = false;
    if (!str->isEqualTo("IOUserClientClass")) {
        ret = super::setProperty(aKey, anObject);
    }
    OSSafeReleaseNULL(str);
    return ret;
}

bool VoodooInputWellspringSimulator::setProperty(const char *aKey, void *bytes, unsigned int length) {
    return super::setProperty(aKey, bytes, length);
}

bool VoodooInputWellspringSimulator::setProperty(const char *aKey, unsigned long long aValue, unsigned int aNumberOfBits) {
    return super::setProperty(aKey, aValue, aNumberOfBits);
}

bool VoodooInputWellspringSimulator::setProperty(const char *aKey, const char *aString) {
    IOLog("MT1Sim: Attempting to set String Property! %s\n", aKey);
    OSString *str = OSString::withCString(aKey);
    bool ret = false;
    if (!str->isEqualTo("IOUserClientClass")) {
        ret = super::setProperty(aKey, aString);
    }
    OSSafeReleaseNULL(str);
    return ret;
}

bool VoodooInputWellspringSimulator::setProperty(const OSSymbol *aKey, OSObject *anObject) {
    IOLog("MT1Sim: Attempting to set OSSymbol Object Property! %s\n", aKey->getCStringNoCopy());
    bool ret = false;
    if (!aKey->isEqualTo("IOUserClientClass")) {
        ret = super::setProperty(aKey, anObject);
    }
    return ret;
}

bool VoodooInputWellspringSimulator::setProperty(const OSString *aKey, OSObject *anObject) {
    // Hidd tries to set a new user client, so we block setProperty writes to the user client key.
    IOLog("MT1Sim: Attempting to set OSString Object Property! %s\n", aKey->getCStringNoCopy());
    bool ret = false;
    if (!aKey->isEqualTo("IOUserClientClass")) {
        ret = super::setProperty(aKey, anObject);
    }
    return ret;
}

bool VoodooInputWellspringSimulator::init(OSDictionary *props) {
    bool success = super::init(props);
    
    if (!success) {
        return false;
    }
    
    setProperty("Clicking", kOSBooleanTrue);
    setProperty("TrackpadScroll", kOSBooleanTrue);
    setProperty("TrackpadHorizScroll", kOSBooleanTrue);
    setProperty("AlwaysNeedsVelocityCalculated", kOSBooleanTrue);
    setProperty("Endianness", MT1_LITTLE_ENDIAN, 32);
    setProperty("Multitouch ID", 0x30000001d183000, 64);
    setProperty("Family ID", 0x62, 8);
    setProperty("bcdVersion", 0x109, 16);
    setProperty("Max Packet Size", 0x200, 32);
    
    if (version_major >= 16) {
        setProperty("SupportsGestureScrolling", kOSBooleanTrue);
        setProperty("ApplePreferenceIdentifier", "com.apple.AppleMultitouchTrackpad");
        setProperty("MT Built-in", kOSBooleanTrue);
        setProperty("ApplePreferenceCapability", kOSBooleanTrue);
        setProperty("TrackpadEmbedded", kOSBooleanTrue);
    }
    
    setProperty("TrackpadCornerSecondaryClick", kOSBooleanTrue);
    
    OSDictionary* trackpadPrefs = OSDictionary::withCapacity(1);
    setProperty("TrackpadUserPreferences", trackpadPrefs);
    OSSafeReleaseNULL(trackpadPrefs);
    return true;
}

bool VoodooInputWellspringSimulator::start(IOService *provider) {
    if (!super::start(provider)) return false;
    
    engine = OSDynamicCast(VoodooInput, provider);
    if (!engine) {
        return false;
    }
    
    setProperty("Sensor Surface Width", engine->getPhysicalMaxX(), 32);
    setProperty("Sensor Surface Height", engine->getPhysicalMaxY(), 32);
    setProperty("Sensor Region Param", const_cast<UInt8 *>(MTSensorParams), SensorParamsLength);
    setProperty("Sensor Region Descriptor", const_cast<UInt8 *>(MTSensorDesc), SensorDescLength);
    
    workloop = getWorkLoop();
    if (workloop == nullptr) {
        IOLog("MT1Sim: Failed to create work loop!\n");
        return false;
    }
    
    workloop->retain();
    
    cmdGate = IOCommandGate::commandGate(this);
    if (cmdGate == nullptr) {
        IOLog("MT1Sim: Failed to create cmd gate\n");
        return false;
    }
    
    workloop->addEventSource(cmdGate);
    
    userClients = OSSet::withCapacity(1);
    if (userClients == nullptr) {
        IOLog("MT1Sim: Failed to create user clients array\n");
        return false;
    }
    
    size_t inputSize = sizeof(WELLSPRING3_REPORT) + (sizeof(WELLSPRING3_FINGER) * VOODOO_INPUT_MAX_TRANSDUCERS);
    inputReport = reinterpret_cast<WELLSPRING3_REPORT *>(IOMalloc(inputSize));
    bzero(inputReport, inputSize);
    
    if (inputReport == nullptr) return false;
    
    clock_get_uptime(&startTimestamp);
    
    IOServiceMatchingNotificationHandler notificationHandler = OSMemberFunctionCast(IOServiceMatchingNotificationHandler, this, &VoodooInputWellspringSimulator::notificationEventDriver);
    
    OSDictionary *matching = serviceMatching("AppleUSBMultitouchHIDEventDriver");
    propertyMatching(OSSymbol::withCString("LocationID"), newLocationIDNumber(), matching);
    eventDriverPublish = addMatchingNotification(gIOFirstPublishNotification, matching, notificationHandler, this);
    eventDriverTerminate = addMatchingNotification(gIOTerminatedNotification, matching, notificationHandler, this);
    OSSafeReleaseNULL(matching);
    
    registerService();
    
    return true;
}

void VoodooInputWellspringSimulator::notificationEventDriver(IOService * newService, IONotifier * notifier) {
    IOLog("%s Event Driver Published/Terminated\n", getName());
    if (notifier == eventDriverPublish && eventDriver == nullptr) {
        eventDriver = OSDynamicCast(AppleUSBMultitouchHIDEventDriver, newService);
        if (eventDriver != nullptr) eventDriver->retain();
        else IOLog("%s is null!?!?!?!\n", getName());
    } else if (notifier == eventDriverTerminate && eventDriver != nullptr) {
        eventDriver->release();
        eventDriver = nullptr;
    }
}

void VoodooInputWellspringSimulator::stop(IOService *provider) {
    if (eventDriverPublish != nullptr) {
        eventDriverPublish->remove();
    }
    
    if (eventDriverTerminate != nullptr) {
        eventDriverTerminate->remove();
    }
    
    if (workloop) {
        workloop->removeEventSource(cmdGate);
    }
    
    if (userClients != nullptr) {
        userClients->flushCollection();
        OSSafeReleaseNULL(userClients);
    }
    
    if (inputReport != nullptr) {
        size_t inputSize = sizeof(WELLSPRING3_REPORT) + (sizeof(WELLSPRING3_FINGER) * VOODOO_INPUT_MAX_TRANSDUCERS);
        IOFree(inputReport, inputSize);
    }
    
    OSSafeReleaseNULL(eventDriverPublish);
    OSSafeReleaseNULL(eventDriverTerminate);
    OSSafeReleaseNULL(workloop);
    OSSafeReleaseNULL(cmdGate);
    super::stop(provider);
}

const unsigned char report_descriptor[] = {
    0x06, 0x00, 0xFF, 0x09, 0x01, 0xA1, 0x03, 0x06,
    0x00, 0xFF, 0x09, 0x01, 0x15, 0x00, 0x26, 0xFF,
    0x00, 0x85, 0x44, 0x75, 0x08, 0x96, 0xFF, 0x01,
    0x81, 0x00, 0xC0
};

IOReturn VoodooInputWellspringSimulator::newReportDescriptor(IOMemoryDescriptor** descriptor) const {
    IOBufferMemoryDescriptor* report_descriptor_buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof(report_descriptor));
    
    if (!report_descriptor_buffer) {
        IOLog("%s Could not allocate buffer for report descriptor\n", getName());
        return kIOReturnNoResources;
    }
    
    report_descriptor_buffer->writeBytes(0, report_descriptor, sizeof(report_descriptor));
    *descriptor = report_descriptor_buffer;
    
    return kIOReturnSuccess;
}

OSNumber* VoodooInputWellspringSimulator::newPrimaryUsageNumber() const {
    return OSNumber::withNumber(kHIDUsage_GD_Mouse, 32);
}

OSNumber* VoodooInputWellspringSimulator::newPrimaryUsagePageNumber() const {
    return OSNumber::withNumber(kHIDPage_GenericDesktop, 32);
}

OSNumber* VoodooInputWellspringSimulator::newProductIDNumber() const {
    return OSNumber::withNumber(0x252, 32);
}

OSString* VoodooInputWellspringSimulator::newProductString() const {
    return OSString::withCString("Wellspring3 Emulator");
}

OSString* VoodooInputWellspringSimulator::newSerialNumberString() const {
    return OSString::withCString("None");
}

OSString* VoodooInputWellspringSimulator::newTransportString() const {
    return OSString::withCString("USB");
}

OSNumber* VoodooInputWellspringSimulator::newVendorIDNumber() const {
    return OSNumber::withNumber(0x5ac, 16);
}

OSNumber* VoodooInputWellspringSimulator::newLocationIDNumber() const {
    return OSNumber::withNumber(0x1d183000, 32);
}

OSNumber* VoodooInputWellspringSimulator::newVersionNumber() const {
    return OSNumber::withNumber(0x219, 32);
}

bool VoodooInputWellspringSimulator::registerUserClient(IOService *client) {
    IOLog("MT1Sim: Adding user client\n");
    return userClients->setObject(client);
}

void VoodooInputWellspringSimulator::unregisterUserClient(IOService *client) {
    IOLog("MT1Sim: Removing user client\n");
    userClients->removeObject(client);
}

IOReturn VoodooInputWellspringSimulator::getReport(MTDeviceReportStruct *toFill) {
    UInt32 width, height;
    
    switch (toFill->reportId) {
        case MT1ReportDetectConfig:
            toFill->data[0] = 0x08;
            toFill->dataSize = 1;
            break;
        case MT1ReportSensorParam:
            memcpy(toFill->data, MTSensorParams, SensorParamsLength);
            toFill->dataSize = SensorParamsLength;
            break;
        case MT1ReportSensorDescriptor:
            memcpy(toFill->data, MTSensorDesc, SensorDescLength);
            toFill->dataSize = SensorDescLength;
            break;
        case MT1ReportSensorSize:
            width = engine->getPhysicalMaxX();
            height = engine->getPhysicalMaxY();
            
            for (int i = 0; i < sizeof(UInt32); i++) {
                toFill->data[i] = width & 0xFF;
                width >>= 8;
            }
            
            for (int i = 4; i < sizeof(UInt32) + 4; i++) {
                toFill->data[i] = height & 0xFF;
                height >>= 8;
            }
            
            toFill->dataSize = sizeof(UInt32) * 2;
            break;
        case MT1ReportSensorRows:
            memcpy(toFill->data, MTSensorRows, SensorRowsLength);
            toFill->dataSize = SensorRowsLength;
            break;
        case MT1ReportFamilyId:
            toFill->data[0] = FamilyID;
            toFill->dataSize = FamilyIDLength;
            break;
            
        default: return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

void VoodooInputWellspringSimulator::constructButtonReport(UInt8 btnState) {
    MTRelativePointerReport report;
    AbsoluteTime timestamp;
    
    clock_get_uptime(&timestamp);
    
    if (btnState == lastButtonState) return;
    lastButtonState = btnState;
    
    // macOS Sierra changed how button handling works
    // There is now a hid report to send into the abyss of the MT stack
    if (version_major >= 17) {
        bzero(&report, sizeof(MTRelativePointerReport));
        
        report.Buttons = btnState;
        report.ReportID = 0x82;
        report.Unknown1 = 1;
        report.Timestamp = timestamp;
        OSCollectionIterator *iter = OSCollectionIterator::withCollection(userClients);
        while (VoodooInputWellspringUserClient *client = OSDynamicCast(VoodooInputWellspringUserClient, iter->getNextObject())) {
            client->enqueueData(&report, sizeof(MTRelativePointerReport));
        }
        OSSafeReleaseNULL(iter);
    } else if (eventDriver != nullptr) {
        // TODO: There is some weird logic to do with the "mapClick" property.
        // I'm not even sure this is ever set though, so this is a don't care for now!
        
        eventDriver->dispatchRelativePointerEvent(timestamp, 0, 0, btnState, 0);
    }
}

void VoodooInputWellspringSimulator::constructReport(VoodooInputEvent& event) {
    AbsoluteTime timestamp = event.timestamp;
    size_t inputSize = sizeof(WELLSPRING3_REPORT) + (sizeof(WELLSPRING3_FINGER) * event.contact_count);
    
    if (inputReport == nullptr) return;
    
    inputReport->ReportID = 0x74;
    inputReport->Counter++;
    inputReport->unkown1 = 0x03;
    inputReport->HeaderSize = sizeof(WELLSPRING3_REPORT);
    inputReport->unk2[0] =  0x00; // Magic
    inputReport->unk2[1] =  0x17;
    inputReport->unk2[2] =  0x07;
    inputReport->unk2[3] =  0x97;
    inputReport->TotalFingerDataSize = sizeof(WELLSPRING3_FINGER) * event.contact_count;
    inputReport->NumFingers = event.contact_count;
    inputReport->Button = event.transducers[0].isPhysicalButtonDown || event.transducers[0].currentCoordinates.pressure > 100;
    inputReport->unknown1 = 0x00000010;
    
    constructButtonReport(inputReport->Button);

    // rotation check
    UInt8 transform = engine->getTransformKey();
    
    // timestamp
    AbsoluteTime relativeTimestamp = timestamp;
    
    SUB_ABSOLUTETIME(&relativeTimestamp, &startTimestamp);
    
    UInt64 milliTimestamp;
    
    absolutetime_to_nanoseconds(relativeTimestamp, &milliTimestamp);
    
    milliTimestamp /= 1000000;
    inputReport->Timestamp = static_cast<UInt32>(milliTimestamp);
    
    // finger data
    bool input_active = inputReport->Button;
    bool is_error_input_active = false;
    
    for (int i = 0; i < event.contact_count; i++) {
        const VoodooInputTransducer &transducer = event.transducers[i];

         if (!transducer.isValid)
            continue;

        if (transducer.type == VoodooInputTransducerType::STYLUS) {
            continue;
        }

        // in case the obtained id is greater than 14, usually 0~4 for common devices.
        UInt16 touch_id = transducer.secondaryId % 15;
        input_active |= transducer.isTransducerActive;

        WELLSPRING3_FINGER& fingerData = inputReport->Fingers[i];

        IOFixed scaled_x = ((transducer.currentCoordinates.x * 1.0f) / engine->getLogicalMaxX()) * MT1_MAX_X;
        IOFixed scaled_y = ((transducer.currentCoordinates.y * 1.0f) / engine->getLogicalMaxY()) * MT1_MAX_Y;

        if (scaled_x < 1 && scaled_y >= MT1_MAX_Y) {
            is_error_input_active = true;
        }
        
        if (transform) {
            if (transform & kIOFBSwapAxes) {
                scaled_x = ((transducer.currentCoordinates.y * 1.0f) / engine->getLogicalMaxY()) * MT1_MAX_X;
                scaled_y = ((transducer.currentCoordinates.x * 1.0f) / engine->getLogicalMaxX()) * MT1_MAX_Y;
            }
            
            if (transform & kIOFBInvertX) {
                scaled_x = MT1_MAX_X - scaled_x;
            }
            if (transform & kIOFBInvertY) {
                scaled_y = MT1_MAX_Y - scaled_y;
            }
        }
        
        scaled_y = MT1_MAX_Y - scaled_y;

        fingerData.State = touchActive[touch_id] ? kTouchStateActive : kTouchStateStart;
        touchActive[touch_id] = transducer.isTransducerActive || transducer.isPhysicalButtonDown;

        fingerData.Finger = transducer.fingerType;

        if (transducer.supportsPressure) {
            fingerData.Size = transducer.currentCoordinates.width * 4;
            fingerData.ToolMajor = transducer.currentCoordinates.width * 16;
            fingerData.ToolMinor = transducer.currentCoordinates.width * 16;
        } else {
            fingerData.Size = 200;
            fingerData.ToolMajor = 800;
            fingerData.ToolMinor = 800;
        }
        
        if (!transducer.isTransducerActive && !transducer.isPhysicalButtonDown) {
            touchActive[touch_id] = false;
            fingerData.State = kTouchStateStop;
            fingerData.Size = 0x0;
            fingerData.ToolMajor = 0;
            fingerData.ToolMinor = 0;
        }

        fingerData.X = (SInt16)(scaled_x - (MT1_MAX_X / 2));
        fingerData.Y = (SInt16)(scaled_y);
        fingerData.Unknown = touchActive[touch_id] ? 0xFF : 0; // Always set to 1 in MT2 logic, though my MBP9,2 sets this to 0xFF once it starts moving
        
        fingerData.Orientation = 0x4000; // pi/2
        fingerData.Id = touch_id + 1;
    }

//    inputReport->TouchActive = input_active;
    
    if (!is_error_input_active) {
        IOLog("MT1Sim: Sending report\n");
        enqueueData(inputReport, (UInt32) inputSize);
    }
    
    if (!input_active) {
        memset(touchActive, false, sizeof(touchActive));
        constructButtonReport(0);
        
        // Stop finger
        inputReport->Counter++;
        inputReport->Fingers[0].State = kTouchStateStop;
        inputReport->Fingers[0].Size = 0x0;
        inputReport->Fingers[0].ToolMajor = 0x0;
        inputReport->Fingers[0].ToolMinor = 0x0;

        inputReport->Counter++;
        inputReport->Timestamp += 10;
        enqueueData(inputReport, inputSize);

        // Undefined/Zeroed out finger
        inputReport->Counter++;
        inputReport->Timestamp += 10;
        inputReport->Fingers[0].Finger = kMT2FingerTypeUndefined;
        inputReport->Fingers[0].State = kTouchStateInactive;
        enqueueData(inputReport, inputSize);

        // Blank report
        inputReport->NumFingers = 0;
        inputReport->TotalFingerDataSize = 0;
        inputReport->Counter++;
        inputReport->Timestamp += 10;
        enqueueData(inputReport, sizeof(WELLSPRING3_REPORT));
    }
    
    bzero(inputReport->Fingers, inputReport->TotalFingerDataSize);
}

void VoodooInputWellspringSimulator::enqueueData(WELLSPRING3_REPORT *report, size_t dataLen) {
#if DEBUG
    IOLog("%s Sending report with button %d, finger count %hhu, at %dms\n", getName(), report->Button, report->NumFingers, report->Timestamp);
    for (size_t i = 0; i < report->NumFingers; i++) {
        WELLSPRING3_FINGER &f = report->Fingers[i];
        IOLog("%s [%zu] (%d, %d) (%d, %d)dx F%d St%d Maj%d Min%d Sz%d ID%d A%d\n", getName(), i, f.X, f.Y, f.XVelocity, f.YVelocity, f.Finger, f.State, f.ToolMajor, f.ToolMinor, f.Size, f.Id, f.Orientation);
    }
#endif
    
    OSCollectionIterator *iter = OSCollectionIterator::withCollection(userClients);
    while (VoodooInputWellspringUserClient *client = OSDynamicCast(VoodooInputWellspringUserClient, iter->getNextObject())) {
        client->enqueueData(report, dataLen);
    }
    OSSafeReleaseNULL(iter);
}
