//
//  AppleUSBMultitouchDriver.cpp
//  VoodooInput
//
//  Created by Avery Black on 12/31/22.
//  Copyright © 2022 Kishor Prins. All rights reserved.
//

#include "VoodooInputMT1Simulator.hpp"
#include <libkern/version.h>

#define super IOHIDDevice
OSDefineMetaClassAndStructors(VoodooInputMT1Simulator, IOHIDDevice);

bool VoodooInputMT1Simulator::setProperty(const char *aKey, OSObject *anObject) {
    IOLog("MT1Sim: Attempting to set Property! %s\n", aKey);
    OSString *str = OSString::withCString(aKey);
    bool ret = false;
    if (!str->isEqualTo("IOUserClientClass")) {
        ret = super::setProperty(aKey, anObject);
    }
    OSSafeReleaseNULL(str);
    return ret;
}

bool VoodooInputMT1Simulator::setProperty(const char *aKey, unsigned long long aValue, unsigned int aNumberOfBits) {
    return super::setProperty(aKey, aValue, aNumberOfBits);
}

bool VoodooInputMT1Simulator::setProperty(const char *aKey, const char *aString) {
    IOLog("MT1Sim: Attempting to set String Property! %s\n", aKey);
    OSString *str = OSString::withCString(aKey);
    bool ret = false;
    if (!str->isEqualTo("IOUserClientClass")) {
        ret = super::setProperty(aKey, aString);
    }
    OSSafeReleaseNULL(str);
    return ret;
}

bool VoodooInputMT1Simulator::setProperty(const OSSymbol *aKey, OSObject *anObject) {
    IOLog("MT1Sim: Attempting to set OSSymbol Object Property! %s\n", aKey->getCStringNoCopy());
    bool ret = false;
    if (!aKey->isEqualTo("IOUserClientClass")) {
        ret = super::setProperty(aKey, anObject);
    }
    return ret;
}

bool VoodooInputMT1Simulator::setProperty(const OSString *aKey, OSObject *anObject) {
    IOLog("MT1Sim: Attempting to set OSString Object Property! %s\n", aKey->getCStringNoCopy());
    bool ret = false;
    if (!aKey->isEqualTo("IOUserClientClass")) {
        ret = super::setProperty(aKey, anObject);
    }
    return ret;
}

bool VoodooInputMT1Simulator::init(OSDictionary *props) {
    // Hidd tries to set a new user client, so we block setProperty writes to the user client key.
    // This means that we need to set the user here in the props dictionary
    props = OSDictionary::withCapacity(1);
    OSString* userClient = OSString::withCString("VoodooInputMT1UserClient");
    props->setObject(kIOUserClientClassKey, userClient);
    props->setObject("Clicking", kOSBooleanTrue);
    props->setObject("TrackpadScroll", kOSBooleanTrue);
    props->setObject("TrackpadHorizScroll", kOSBooleanTrue);
    bool success = super::init(props);
    OSSafeReleaseNULL(userClient);
    OSSafeReleaseNULL(props);
    
    if (!success) {
        return false;
    }
    
    setProperty("MTHIDDevice", kOSBooleanTrue);
    setProperty("Multitouch ID", 0x30000001d183000, 64);
    setProperty("Family ID", 0x62, 8);
    setProperty("bcdVersion", 0x109, 16);
    setProperty("parser-type", 0x3e8, 32);
    setProperty("parser-options", 39, 32);
    setProperty("HIDDefaultBehavior", "Mouse");
    setProperty("HIDServiceSupport", kOSBooleanTrue);
    setProperty("Max Packet Size", 0x200, 32);
    
    if (version_major >= 16) {
        setProperty("SupportsGestureScrolling", kOSBooleanTrue);
        setProperty("TrackpadFourFingerGestures", kOSBooleanTrue);
        setProperty("ApplePreferenceIdentifier", "com.apple.AppleMultitouchTrackpad");
        setProperty("MT Built-in", kOSBooleanTrue);
        setProperty("ApplePreferenceCapability", kOSBooleanTrue);
        setProperty("TrackpadEmbedded", kOSBooleanTrue);
        setProperty("TrackpadThreeFingerDrag", kOSBooleanTrue);
    }
    
    setProperty("TrackpadCornerSecondaryClick", kOSBooleanTrue);
    OSDictionary* dict = OSDictionary::withCapacity(1);
    OSString* str = OSString::withCString("AppleMultitouchDriver.kext/Contents/PlugIns/MultitouchHID.plugin");
    dict->setObject("0516B563-B15B-11DA-96EB-0014519758EF", str);
    
    setProperty(kIOCFPlugInTypesKey, dict);
    OSSafeReleaseNULL(str);
    OSSafeReleaseNULL(dict);
    
    OSDictionary* trackpadPrefs = OSDictionary::withCapacity(1);
    setProperty("TrackpadUserPreferences", trackpadPrefs);
    OSSafeReleaseNULL(trackpadPrefs);
    return true;
}

bool VoodooInputMT1Simulator::start(IOService *provider) {
    if (!super::start(provider)) return false;
    
    engine = OSDynamicCast(VoodooInput, provider);
    if (!engine) {
        return false;
    }
    
    setProperty("Sensor Surface Width", engine->getPhysicalMaxX(), 32);
    setProperty("Sensor Surface Height", engine->getPhysicalMaxY(), 32);
    
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
    
    clock_get_uptime(&startTimestamp);
    
    registerService();
    
    return true;
}

void VoodooInputMT1Simulator::stop(IOService *provider) {
    if (workloop) {
        workloop->removeEventSource(cmdGate);
    }
    
    OSSafeReleaseNULL(workloop);
    OSSafeReleaseNULL(cmdGate);
    if (userClients != nullptr) {
        userClients->flushCollection();
        OSSafeReleaseNULL(userClients);
    }
    
    super::stop(provider);
}

const unsigned char report_descriptor[] = {
    0x06, 0x00, 0xFF, 0x09, 0x01, 0xA1, 0x03, 0x06,
    0x00, 0xFF, 0x09, 0x01, 0x15, 0x00, 0x26, 0xFF,
    0x00, 0x85, 0x44, 0x75, 0x08, 0x96, 0xFF, 0x01,
    0x81, 0x00, 0xC0
};

IOReturn VoodooInputMT1Simulator::newReportDescriptor(IOMemoryDescriptor** descriptor) const {
    IOBufferMemoryDescriptor* report_descriptor_buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof(report_descriptor));
    
    if (!report_descriptor_buffer) {
        IOLog("%s Could not allocate buffer for report descriptor\n", getName());
        return kIOReturnNoResources;
    }
    
    report_descriptor_buffer->writeBytes(0, report_descriptor, sizeof(report_descriptor));
    *descriptor = report_descriptor_buffer;
    
    return kIOReturnSuccess;
}

OSNumber* VoodooInputMT1Simulator::newPrimaryUsageNumber() const {
    return OSNumber::withNumber(kHIDUsage_GD_Mouse, 32);
}

OSNumber* VoodooInputMT1Simulator::newPrimaryUsagePageNumber() const {
    return OSNumber::withNumber(kHIDPage_GenericDesktop, 32);
}

OSNumber* VoodooInputMT1Simulator::newProductIDNumber() const {
    return OSNumber::withNumber(0x252, 32);
}

OSString* VoodooInputMT1Simulator::newProductString() const {
    return OSString::withCString("Apple Internal Trackpad");
}

OSString* VoodooInputMT1Simulator::newSerialNumberString() const {
    return OSString::withCString("None");
}

OSString* VoodooInputMT1Simulator::newTransportString() const {
    return OSString::withCString("USB");
}

OSNumber* VoodooInputMT1Simulator::newVendorIDNumber() const {
    return OSNumber::withNumber(0x5ac, 16);
}

OSNumber* VoodooInputMT1Simulator::newLocationIDNumber() const {
    return OSNumber::withNumber(0x1d183000, 32);
}

OSNumber* VoodooInputMT1Simulator::newVersionNumber() const {
    return OSNumber::withNumber(0x219, 32);
}

bool VoodooInputMT1Simulator::registerUserClient(IOService *client) {
    IOLog("MT1Sim: Adding user client\n");
    return userClients->setObject(client);
}

void VoodooInputMT1Simulator::unregisterUserClient(IOService *client) {
    IOLog("MT1Sim: Removing user client\n");
    userClients->removeObject(client);
}

// Reports come from a MacbookPro9,2
#define SensorParamsLength 6
static const UInt8 MT1SensorParams[SensorParamsLength] = {0x00, 0x00, 0x03, 0x00, 0xD6, 0x01};

#define SensorDescLength 8
static const UInt8 MT1SensorDesc[SensorDescLength] = {0x01, 0x01, 0x00, 0x0c, 0x01, 0x00, 0x14, 0x00};

#define SensorRowsLength 5
static const UInt8 MT1SensorRows[SensorRowsLength] = {
    0x01,       // Endianness (no clue what 1 means)
    0x0C,       // Rows
    0x14,       // Columns
    0x01, 0x09  // BCD Version
};

#define FamilyID 0x62
#define FamilyIDLength 1

IOReturn VoodooInputMT1Simulator::getReport(MT1DeviceReportStruct *toFill) {
    UInt32 width, height;
    
    switch (toFill->reportId) {
        case MT1ReportDetectConfig:
            toFill->data[0] = 0x08;
            toFill->dataSize = 1;
            break;
        case MT1ReportSensorParam:
            memcpy(toFill->data, MT1SensorParams, SensorParamsLength);
            toFill->dataSize = SensorParamsLength;
            break;
        case MT1ReportSensorDescriptor:
            memcpy(toFill->data, MT1SensorDesc, SensorDescLength);
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
            memcpy(toFill->data, MT1SensorRows, SensorRowsLength);
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

void VoodooInputMT1Simulator::constructReport(VoodooInputEvent& event) {
    AbsoluteTime timestamp = event.timestamp;
    size_t inputSize = sizeof(MT1_INPUT_REPORT) + (sizeof(MT1_INPUT_REPORT_FINGER) * event.contact_count);
    MT1_INPUT_REPORT *inputReport = reinterpret_cast<MT1_INPUT_REPORT *>(IOMalloc(inputSize));
    
    IOLog("MT1Sim: Constructing report\n");
    if (inputReport == nullptr) return;
    
    inputReport->ReportID = 0x28;
    inputReport->Timestamp = 0;
    inputReport->Button = event.transducers[0].isPhysicalButtonDown;

    // rotation check
    UInt8 transform = engine->getTransformKey();
    
    // timestamp
    AbsoluteTime relativeTimestamp = timestamp;
    
    SUB_ABSOLUTETIME(&relativeTimestamp, &startTimestamp);
    
    UInt64 milliTimestamp;
    
    absolutetime_to_nanoseconds(relativeTimestamp, &milliTimestamp);
    
    milliTimestamp /= 1000000;
    inputReport->Timestamp = milliTimestamp;
    
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

        MT1_INPUT_REPORT_FINGER& fingerData = inputReport->FINGERS[i];

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

        fingerData.State = touchActive[touch_id] ? kTouchStateActive : kTouchStateStart;
        touchActive[touch_id] = transducer.isTransducerActive || transducer.isPhysicalButtonDown;

        fingerData.Finger = transducer.fingerType;

        if (transducer.supportsPressure) {
            fingerData.Pressure = transducer.currentCoordinates.pressure;
            fingerData.Size = transducer.currentCoordinates.width;
            fingerData.Touch_Major = transducer.currentCoordinates.width;
            fingerData.Touch_Minor = transducer.currentCoordinates.width;
        } else {
            fingerData.Pressure = 5;
            fingerData.Size = 10;
            fingerData.Touch_Major = 20;
            fingerData.Touch_Minor = 20;
        }
        
        if (!transducer.isTransducerActive && !transducer.isPhysicalButtonDown) {
            fingerData.State = kTouchStateStop;
            fingerData.Size = 0x0;
            fingerData.Touch_Minor = 0;
            fingerData.Touch_Major = 0;
        }

        fingerData.X = (SInt16)(scaled_x - (MT1_MAX_X / 2));
        fingerData.Y = (SInt16)(scaled_y - (MT1_MAX_Y / 2)) * -1;
        
        fingerData.Orientation = 0x4; // pi/2
        fingerData.Identifier = touch_id + 1;
    }

    inputReport->TouchActive = input_active;
    
    if (!is_error_input_active) {
        OSCollectionIterator* iter = OSCollectionIterator::withCollection(userClients);
        while (VoodooInputMT1UserClient *client = OSDynamicCast(VoodooInputMT1UserClient, iter->getNextObject())) {
            IOLog("MT1Sim: Sending report\n");
            client->enqueueData(inputReport, inputSize);
        }
        OSSafeReleaseNULL(iter);
    }
    
    if (!input_active) {
        memset(touchActive, false, sizeof(touchActive));

        inputReport->FINGERS[0].Size = 0x0;
        inputReport->FINGERS[0].Touch_Major = 0x0;
        inputReport->FINGERS[0].Touch_Minor = 0x0;

        milliTimestamp += 10;
        inputReport->Timestamp = milliTimestamp;
        
        OSCollectionIterator* iter = OSCollectionIterator::withCollection(userClients);
        while (VoodooInputMT1UserClient *client = OSDynamicCast(VoodooInputMT1UserClient, iter->getNextObject())) {
            client->enqueueData(inputReport, inputSize);
        }
        OSSafeReleaseNULL(iter);

        inputReport->FINGERS[0].Finger = kMT2FingerTypeUndefined;
        inputReport->FINGERS[0].State = kTouchStateInactive;
        iter = OSCollectionIterator::withCollection(userClients);
        while (VoodooInputMT1UserClient *client = OSDynamicCast(VoodooInputMT1UserClient, iter->getNextObject())) {
            client->enqueueData(inputReport, inputSize);
        }
        OSSafeReleaseNULL(iter);

        // Blank report
        milliTimestamp += 10;
        inputReport->Timestamp = milliTimestamp;
        iter = OSCollectionIterator::withCollection(userClients);
        while (VoodooInputMT1UserClient *client = OSDynamicCast(VoodooInputMT1UserClient, iter->getNextObject())) {
            client->enqueueData(inputReport, sizeof(MT1_INPUT_REPORT));
        }
        OSSafeReleaseNULL(iter);
    }
    
    IOFree(inputReport, inputSize);
}
