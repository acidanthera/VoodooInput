//
//  VoodooInput.hpp
//  VoodooInput
//
//  Copyright © 2019 Kishor Prins. All rights reserved.
//

#ifndef VOODOO_INPUT_HPP
#define VOODOO_INPUT_HPP

#include <IOKit/IOService.h>

class VoodooInputSimulatorDevice;
class VoodooInputActuatorDevice;
class VoodooInputWellspringSimulator;
class TrackpointDevice;

#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

/* State bits reference: linux/drivers/hid/hid-magicmouse.c#L58-L63 */
#define MT2_TOUCH_STATE_BIT_TRANSITION (0x1)
#define MT2_TOUCH_STATE_BIT_NEAR (0x1 << 1)
#define MT2_TOUCH_STATE_BIT_CONTACT (0x1 << 2)

enum TouchStates {
    kTouchStateInactive = 0x0,
    kTouchStateStart = MT2_TOUCH_STATE_BIT_NEAR | MT2_TOUCH_STATE_BIT_TRANSITION,
    kTouchStateActive = MT2_TOUCH_STATE_BIT_CONTACT,
    kTouchStateStop = MT2_TOUCH_STATE_BIT_CONTACT | MT2_TOUCH_STATE_BIT_NEAR | MT2_TOUCH_STATE_BIT_TRANSITION
};

class EXPORT VoodooInput : public IOService {
    OSDeclareDefaultStructors(VoodooInput);
    
    IOService* parentProvider;
    
    VoodooInputWellspringSimulator* simulator;
    VoodooInputActuatorDevice* actuator;
    TrackpointDevice* trackpoint;
    
    UInt8 transformKey;
    
    UInt32 logicalMaxX = 0;
    UInt32 logicalMaxY = 0;
    UInt32 physicalMaxX = 0;
    UInt32 physicalMaxY = 0;
public:
    bool start(IOService* provider) override;
    void stop(IOService* provider) override;
    bool willTerminate(IOService* provider, IOOptionBits options) override;
    
    UInt8 getTransformKey();

    UInt32 getPhysicalMaxX();
    UInt32 getPhysicalMaxY();

    UInt32 getLogicalMaxX();
    UInt32 getLogicalMaxY();
    
    bool isSierraOrNewer();

    bool updateProperties();

    IOReturn message(UInt32 type, IOService *provider, void *argument) override;
};

#endif
