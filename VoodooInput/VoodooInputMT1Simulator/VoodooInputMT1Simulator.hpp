//
//  AppleUSBMultitouchDriver.hpp
//  VoodooInput
//
//  Created by Avery Black on 12/31/22.
//  Copyright © 2022 Kishor Prins. All rights reserved.
//

#ifndef AppleUSBMultitouchDriver_hpp
#define AppleUSBMultitouchDriver_hpp

#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>

#include "../VoodooInput.hpp"
#include "../VoodooInputMultitouch/VoodooInputTransducer.h"
#include "../VoodooInputMultitouch/VoodooInputEvent.h"
#include "../VoodooInputMultitouch/MultitouchHelpers.h"

/* Finger Packet
+---+---+---+---+---+---+---+---+---+
|   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+---+
| 0 |           x: SInt13           |
+---+-----------+                   +
| 1 |           |                   |
+---+           +-------------------+
| 2 |           y: SInt13           |
+---+-----------+-----------+       +
| 3 |                       |       |
|   |                       |       |
+---+-----------+-----------+-------+
| 4 |       touchMajor: UInt8       |
+---+-------------------------------+
| 5 |       touchMinor: UInt8       |
+---+-------+-----------------------+
| 6 |  id   |     size: UInt6       |
+---+-------+---------------+-------+
| 7 |  orientation: UInt6   | id: 4 |
+---+---------------+-------+-------+
| 8 |     state     |               |
|   |     UInt4     |               |
+---+---------------+---------------+
*/
struct __attribute__((__packed__)) MT_1_INPUT_REPORT_FINGER {
    SInt16 X: 13;
    SInt16 Y: 13;
    UInt8 : 6;
    UInt8 Touch_Major;
    UInt8 Touch_Minor;
    UInt8 Size: 6;
    UInt8 Identifier: 4;
    UInt8 Orientation: 6;
    UInt8 State: 4;
    UInt8 : 4;
};

// Missing angle and finger (Maybe they are in Byte 3?)

struct __attribute__((__packed__)) MT1_INPUT_REPORT {
    UInt8 ReportID;
    UInt8 Button: 6;
    UInt32 Timestamp: 18;
    
    MT_1_INPUT_REPORT_FINGER FINGERS[]; // May support more fingers
    
    // UInt16 Checksum;
};

static_assert(sizeof(MT1_INPUT_REPORT) == 4, "Unexpected MT1_INPUT_REPORT size");
static_assert(sizeof(MT_1_INPUT_REPORT_FINGER) == 9, "Unexpected MT1_INPUT_REPORT_FINGER size");

// Missing finger type

struct MT1DeviceReportStruct {
    uint8_t reportId;
    uint8_t data[512];
    uint32_t dataSize;
};

#define MT1ReportSensorParam        0xA1
#define MT1ReportSensorDescriptor   0xD0
#define MT1ReportFamilyId           0xD1
#define MT1ReportSensorRows         0xD3
#define MT1ReportSensorSize         0xD9

class EXPORT VoodooInputMT1Simulator : public IOService {
    OSDeclareDefaultStructors(VoodooInputMT1Simulator);
public:
    
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    
    bool registerUserClient(IOService *client);
    void unregisterUserClient(IOService *client);
    
    IOReturn getReport(MT1DeviceReportStruct *toFill);
private:
    OSSet *userClients {nullptr};
    IOWorkLoop *workloop {nullptr};
    IOCommandGate *cmdGate {nullptr};
    VoodooInput *engine {nullptr};
};

#endif /* AppleUSBMultitouchDriver_hpp */
