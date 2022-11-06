/*
 * TrackpointDevice.hpp
 * VoodooTrackpoint
 *
 * Copyright (c) 2019 Leonard Kleinhans <leo-labs>
 *
 */

#ifndef TrackpointDevice_hpp
#define TrackpointDevice_hpp

#include <IOKit/hidsystem/IOHIPointing.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include "VoodooInputMessages.h"
#include "VoodooInputEvent.h"

#define DEFAULT_MULT 64

enum MiddlePressedState {
    NOT_PRESSED,
    PRESSED,
    SCROLLED
};

class TrackpointDevice : public IOHIPointing {
    typedef IOHIPointing super;
    OSDeclareDefaultStructors(TrackpointDevice);
private:
    UInt32 trackpointMult {DEFAULT_MULT};
    UInt32 trackpointScrollMult {DEFAULT_MULT};
    UInt32 trackpointDeadzone {1};
    UInt32 btnCount {3};
    
    MiddlePressedState middleBtnState {NOT_PRESSED};
    int signum(int value);
protected:
    virtual IOItemCount buttonCount() override;
    virtual IOFixed resolution() override;
    
public:
    bool start(IOService* provider) override;
    void stop(IOService* provider) override;
    
    virtual UInt32 deviceType() override;
    virtual UInt32 interfaceID() override;
    
    void updateRelativePointer(int dx, int dy, int buttons, uint64_t timestamp);
    void updateScrollwheel(short deltaAxis1, short deltaAxis2, short deltaAxis3, uint64_t timestamp);
    void updateTrackpointProperties();
    void reportPacket(TrackpointReport &report);
};

#endif /* TrackpointDevice_hpp */
