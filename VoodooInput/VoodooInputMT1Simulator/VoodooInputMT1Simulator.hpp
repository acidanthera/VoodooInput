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
#include <IOKit/hid/IOHIDEventService.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>

#include "./VoodooInputMT1UserClient.hpp"
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

/* Finger Packet
+---+---+---+---+---+---+---+---+---+
|   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+---+
| 0 |           x: SInt13           |
+---+-----------+                   +
| 1 |           |                   |
+---+           +-------------------+
| 2 |           y: SInt13           |
+---+-----------------------+       +
| 3 |          ??           |       |
|   |          ?!           |       |
+---+-----------------------+-------+
| 4 |       touchMajor: UInt8       |
+---+-------------------------------+
| 5 |       touchMinor: UInt8       |
+---+-------+-----------------------+
| 6 |  id   |     size: UInt6       |
+---+-------+---------------+-------+
| 7 |  orientation: UInt6   | id: 4 |
+---+---+-----------+-------+-------+
| 8 | ? |   state   |     finger    |
|   |   |   UInt3   |     UInt4     |
+---+---+-----------+---------------+
*/
//struct __attribute__((__packed__)) MT1_INPUT_REPORT_FINGER {
//    SInt16 X: 13;
//    SInt16 Y: 13;
//    UInt8 Pressure: 6;
//    UInt8 Touch_Major;
//    UInt8 Touch_Minor;
//    UInt8 Size: 6;
//    UInt8 Identifier: 4;
//    UInt8 Orientation: 6;
//    UInt8 Finger: 4;
//    UInt8 State: 3;
//    UInt8 : 1;
//};
//
//struct __attribute__((__packed__)) MT1_INPUT_REPORT {
//    UInt8 ReportID;
//    UInt8 Button: 1;
//    UInt8 TouchActive: 1;
//    UInt32 Timestamp: 22;
//    
//    MT1_INPUT_REPORT_FINGER FINGERS[]; // May support more fingers
//    
//    // UInt16 Checksum;
//};
//
//static_assert(sizeof(MT1_INPUT_REPORT) == 4, "Unexpected MT1_INPUT_REPORT size");
//static_assert(sizeof(MT1_INPUT_REPORT_FINGER) == 9, "Unexpected MT1_INPUT_REPORT_FINGER size");

struct __attribute__((__packed__)) WELLSPRING3_FINGER {
    UInt8 Id;
    UInt8 State;
    UInt8 Finger;
    UInt8 Unknown; // 0 -> 1 -> 0xFF
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
    // ContactDensity can be calculated in MultitouchSupport.plugin
    /*UInt16 ContactDensity;
    UInt16 Unused[2];
    UInt16 Pressure; // Not a thing on Wellspring3 */
};

// There's no finger field

static_assert(sizeof(WELLSPRING3_FINGER) == 20, "Unexpected WELLSPRING3_FINGER size");

struct __attribute__((__packed__)) WELLSPRING3_REPORT {
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
    WELLSPRING3_FINGER Fingers[];
    
    // End of packet always contains Checksum (though this is never checked in MultitouchSupport)
    // UInt16 checksum;
};

static_assert(sizeof(WELLSPRING3_REPORT) == 28, "Unexpected WELLSPRING3_REPORT size");

struct MT1DeviceReportStruct {
    uint8_t reportId;
    uint8_t data[512];
    uint32_t dataSize;
};

#define MT1ReportDetectConfig       0xC8
#define MT1ReportSensorParam        0xA1
#define MT1ReportSensorDescriptor   0xD0
#define MT1ReportFamilyId           0xD1
#define MT1ReportSensorRows         0xD3
#define MT1ReportSensorSize         0xD9

class EXPORT VoodooInputMT1Simulator : public IOHIDDevice {
    OSDeclareDefaultStructors(VoodooInputMT1Simulator);
public:
    virtual bool init(OSDictionary *) override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    
    virtual bool setProperty(const char *aKey, OSObject *anObject) override;
    virtual bool setProperty(const char *aKey, unsigned long long aValue, unsigned int aNumberOfBits) override;
    virtual bool setProperty(const char *aKey, const char *aString) override;
    virtual bool setProperty(const OSSymbol *aKey, OSObject *anObject) override;
    virtual bool setProperty(const OSString *aKey, OSObject *anObject) override;
    
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
    
    IOReturn getReport(MT1DeviceReportStruct *toFill);
    void constructReport(VoodooInputEvent& event);
    void notificationEventDriverPublish(IOService * newService, IONotifier * notifier);
private:
    OSSet *userClients {nullptr};
    bool touchActive[15] {false};
    IOWorkLoop *workloop {nullptr};
    IOCommandGate *cmdGate {nullptr};
    VoodooInput *engine {nullptr};
    AbsoluteTime startTimestamp {};
    WELLSPRING3_REPORT *inputReport{nullptr};
    UInt8 counter {0};
    
    IONotifier *eventDriverPublish {nullptr};
    
    void enqueueData(WELLSPRING3_REPORT *report, size_t dataLen);
};

#endif /* AppleUSBMultitouchDriver_hpp */
