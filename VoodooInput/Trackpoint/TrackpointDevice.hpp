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

enum MiddlePressedState {
    NOT_PRESSED,
    PRESSED,
    SCROLLED
};

class TrackpointDevice : public IOHIPointing {
    typedef IOHIPointing super;
    OSDeclareDefaultStructors(TrackpointDevice);
private:
    int trackpointMultX {1};
    int trackpointMultY {1};
    int trackpointDivX {1};
    int trackpointDivY {1};
    short trackpointScrollMultX {1};
    short trackpointScrollMultY {1};
    short trackpointScrollDivX {1};
    short trackpointScrollDivY {1};
    int trackpointDeadzone {1};
    int btnCount {3};
    
    MiddlePressedState middleBtnState {NOT_PRESSED};
    int signum(int value);
    void getOSIntValue(OSDictionary *dict, int *val, const char *key);
    void getOSShortValue(OSDictionary *dict, short *val, const char *key);
protected:
    virtual IOItemCount buttonCount() override;
    virtual IOFixed resolution() override;
    
public:
    bool start(IOService* provider) override;
    void stop(IOService* provider) override;
    bool willTerminate(IOService* provider, IOOptionBits options) override;
    
    virtual UInt32 deviceType() override;
    virtual UInt32 interfaceID() override;
    
    void updateRelativePointer(int dx, int dy, int buttons, uint64_t timestamp);
    void updateScrollwheel(short deltaAxis1, short deltaAxis2, short deltaAxis3, uint64_t timestamp);
    void updateTrackpointProperties();
    void reportPacket(TrackpointReport &report);
};

#endif /* TrackpointDevice_hpp */
