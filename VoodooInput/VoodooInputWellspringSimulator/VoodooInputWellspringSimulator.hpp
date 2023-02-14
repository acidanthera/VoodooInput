//
//  AppleUSBMultitouchDriver.hpp
//  VoodooInput
//
//  Created by Avery Black on 12/31/22.
//  Copyright © 2022 Kishor Prins. All rights reserved.
//

#ifndef AppleUSBMultitouchDriver_hpp
#define AppleUSBMultitouchDriver_hpp

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>

#include "VoodooInputWellspringEventDriver.hpp"
#include "../VoodooInput.hpp"
#include "../VoodooInputMultitouch/VoodooInputTransducer.h"
#include "../VoodooInputMultitouch/VoodooInputEvent.h"
#include "../VoodooInputMultitouch/MultitouchHelpers.h"

#define MT1_MAX_X 8920
#define MT1_MAX_Y 6600

#define MT1_LITTLE_ENDIAN 1

/* State bits reference: linux/drivers/hid/hid-magicmouse.c#L58-L63 */
#define MT2_TOUCH_STATE_BIT_TRANSITION (0x1)
#define MT2_TOUCH_STATE_BIT_NEAR (0x1 << 1)
#define MT2_TOUCH_STATE_BIT_CONTACT (0x1 << 2)

class VoodooInputWellspringUserClient;

// This report does not come from the hardware, but instead comes from within AppleUSBMultitouch in 10.12+
// This gets sent to userspace on any button presses
struct __attribute__((__packed__)) MTRelativePointerReport {
    UInt8 ReportID;
    UInt8 Unknown1; // Always set to one
    UInt16 Unknown2;
    UInt32 Buttons;
    UInt32 Unknown3[4]; // A dx/dy probably exists in here
    AbsoluteTime Timestamp;
};

struct __attribute__((__packed__)) WELLSPRING_FINGER {
    UInt8 Id;
    UInt8 State;
    UInt8 Finger;
    // Goes from 0 -> 1 -> 0xFF on my MBP9,2
    UInt8 Unknown;
    SInt16 X;
    SInt16 Y;
    // Velocity can be calculated in MultitouchSupport.plugin for us
    // as long as "AlwaysNeedVelocityCalculated" is set.
    SInt16 XVelocity;
    SInt16 YVelocity;
    UInt16 ToolMajor;
    UInt16 ToolMinor;
    UInt16 Orientation;
    UInt16 Size;
    /*
     * ContactDensity can be calculated in MultitouchSupport.plugin
     * UInt16 ContactDensity;
     * UInt16 Unused[2];
     * UInt16 Pressure; // Not a thing on Wellspring3
     */
};

static_assert(sizeof(WELLSPRING_FINGER) == 20, "Unexpected WELLSPRING3_FINGER size");

struct __attribute__((__packed__)) WELLSPRING_REPORT {
    UInt8 ReportID;
    UInt8 Counter; // Unknown, always seems to count up though?
    UInt8 HeaderSize;
    UInt8 unkown1; // Always 3
    UInt32 Timestamp;
    UInt8 unk2[4]; // 0x00, 0x17, 0x07, 0x97
    UInt16 TotalFingerDataSize;
    UInt8 NumFingers;
    UInt8 Button;
    UInt32 unknown1; // 0x00000010
    UInt16 unknown2[4]; // Sometimes Changes
    WELLSPRING_FINGER Fingers[];
};

static_assert(sizeof(WELLSPRING_REPORT) == 28, "Unexpected WELLSPRING3_REPORT size");

struct MTDeviceReportStruct {
    uint8_t reportId;
    uint8_t data[512];
    uint32_t dataSize;
};

#define MT1ReportDetectConfig       0xC8

class EXPORT VoodooInputWellspringSimulator : public IOHIDDevice {
    OSDeclareDefaultStructors(VoodooInputWellspringSimulator);
public:
    virtual bool init(OSDictionary *) override;
    virtual bool start(IOService *provider) override;
    virtual void free() override;
    
    virtual bool setProperty(const char *aKey, OSObject *anObject) override;
    virtual bool setProperty(const char *aKey, unsigned long long aValue, unsigned int aNumberOfBits) override;
    virtual bool setProperty(const char *aKey, const char *aString) override;
    virtual bool setProperty(const OSSymbol *aKey, OSObject *anObject) override;
    virtual bool setProperty(const OSString *aKey, OSObject *anObject) override;
    virtual bool setProperty(const char *aKey, void *bytes, unsigned int length) override;
    
    virtual IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;
    virtual OSNumber* newPrimaryUsageNumber() const override;
    virtual OSNumber* newPrimaryUsagePageNumber() const override;
    virtual OSNumber* newProductIDNumber() const override;
    virtual OSNumber* newVendorIDNumber() const override;
    virtual OSNumber* newLocationIDNumber() const override;
    virtual OSNumber* newVersionNumber() const override;
    virtual OSString* newProductString() const override;
    virtual OSString* newSerialNumberString() const override;
    virtual OSString* newTransportString() const override;

    bool registerUserClient(IOService *client);
    void unregisterUserClient(IOService *client);
    
    IOReturn getReport(MTDeviceReportStruct *toFill);
    void constructReport(VoodooInputEvent& event);
    
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_6
    void notificationEventDriver(IOService * newService, IONotifier * notifier);
#endif
    void notificationEventDriverPublished(IOService *newService);
    void notificationEventDriverTerminated(IOService *terminatedService);

private:
    bool touchActive[15] {false};
    
    IOWorkLoop *workloop {nullptr};
    IOCommandGate *cmdGate {nullptr};
    
    OSSet *userClients {nullptr};
    VoodooInput *engine {nullptr};
    VoodooInputWellspringEventDriver *eventDriver {nullptr};
    
    AbsoluteTime startTimestamp {};
    size_t inputReportSize {};
    WELLSPRING_REPORT *inputReport {nullptr};
    
    IONotifier *eventDriverPublish {nullptr};
    IONotifier *eventDriverTerminate {nullptr};
    
    UInt8 lastButtonState {0};
    
    void enqueueData(WELLSPRING_REPORT *report, size_t dataLen);
    void constructButtonReport(UInt8 btnState);
    void setMTProperties();
    void createEventDriverNotifiers();
};

#endif /* AppleUSBMultitouchDriver_hpp */
