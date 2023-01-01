/*
 * TrackpointDevice.cpp
 * VoodooTrackpoint
 *
 * Copyright (c) 2019 Leonard Kleinhans <leo-labs>
 *
 */

#include "TrackpointDevice.hpp"

OSDefineMetaClassAndStructors(TrackpointDevice, IOHIPointing);

#define abs(x) ((x < 0) ? -(x) : (x))
#define MIDDLE_MOUSE_MASK 0x4

UInt32 TrackpointDevice::deviceType() {
    return NX_EVS_DEVICE_TYPE_MOUSE;
}

UInt32 TrackpointDevice::interfaceID() {
    return NX_EVS_DEVICE_INTERFACE_BUS_ACE;
}

IOItemCount TrackpointDevice::buttonCount() {
    return btnCount;
};

IOFixed TrackpointDevice::resolution() {
    return 400 << 16;
};

bool TrackpointDevice::start(IOService* provider) {
    if (!super::start(provider)) {
        return false;
    }
    
    updateTrackpointProperties();

    setProperty(kIOHIDScrollAccelerationTypeKey, kIOHIDTrackpadScrollAccelerationKey);
    setProperty(kIOHIDScrollResolutionKey, 400 << 16, 32);
    setProperty("HIDScrollResolutionX", 400 << 16, 32);
    setProperty("HIDScrollResolutionY", 400 << 16, 32);

    registerService();
    return true;
}

void TrackpointDevice::getOSIntValue(OSDictionary *dict, int *val, const char *key) {
    OSNumber *osnum = OSDynamicCast(OSNumber, dict->getObject(key));
    if (osnum != nullptr) *val = osnum->unsigned32BitValue();
}

void TrackpointDevice::getOSShortValue(OSDictionary *dict, short *val, const char *key) {
    OSNumber *osnum = OSDynamicCast(OSNumber, dict->getObject(key));
    if (osnum != nullptr) *val = osnum->unsigned16BitValue();
}

void TrackpointDevice::updateTrackpointProperties() {
    OSObject *obj = getProperty(VOODOO_TRACKPOINT_KEY, gIOServicePlane);
    OSDictionary *dict = OSDynamicCast(OSDictionary, obj);
    
    if (dict == nullptr) return;
    
    getOSIntValue(dict, &btnCount, VOODOO_TRACKPOINT_BTN_CNT);
    getOSIntValue(dict, &trackpointDeadzone, VOODOO_TRACKPOINT_DEADZONE);
    getOSIntValue(dict, &trackpointMultX, VOODOO_TRACKPOINT_MOUSE_MULT_X);
    getOSIntValue(dict, &trackpointMultY, VOODOO_TRACKPOINT_MOUSE_MULT_Y);
    getOSIntValue(dict, &trackpointDivX, VOODOO_TRACKPOINT_MOUSE_DIV_X);
    getOSIntValue(dict, &trackpointDivY, VOODOO_TRACKPOINT_MOUSE_DIV_Y);
    
    getOSShortValue(dict, &trackpointScrollMultX, VOODOO_TRACKPOINT_SCROLL_MULT_X);
    getOSShortValue(dict, &trackpointScrollMultY, VOODOO_TRACKPOINT_SCROLL_MULT_Y);
    getOSShortValue(dict, &trackpointScrollDivX, VOODOO_TRACKPOINT_SCROLL_DIV_X);
    getOSShortValue(dict, &trackpointScrollDivY, VOODOO_TRACKPOINT_SCROLL_DIV_Y);
    
    if (trackpointDivX == 0) trackpointDivX = 1;
    if (trackpointDivY == 0) trackpointDivY = 1;
    if (trackpointScrollDivX == 0) trackpointScrollDivX = 1;
    if (trackpointScrollDivY == 0) trackpointScrollDivY = 1;
}

void TrackpointDevice::stop(IOService* provider) {
    super::stop(provider);
}

int TrackpointDevice::signum(int value)
{
    if (value > 0) return 1;
    if (value < 0) return -1;
    return 0;
}

void TrackpointDevice::reportPacket(TrackpointReport &report) {
    SInt32 dx = report.dx;
    SInt32 dy = report.dy;
    UInt32 buttons = report.buttons;
    AbsoluteTime timestamp = report.timestamp;

    dx -= signum(dx) * min(abs(dx), trackpointDeadzone);
    dy -= signum(dy) * min(abs(dy), trackpointDeadzone);

    bool middleBtnNotPressed = (buttons & MIDDLE_MOUSE_MASK) == 0;
    
    // Do not tell macOS about the middle button until it's been released
    // Scrolling with the button down can result in the button being spammed
    switch (middleBtnState) {
        case NOT_PRESSED:
            if (middleBtnNotPressed) {
                break;
            }
            
            middleBtnState = PRESSED;
            /* fallthrough */
        case PRESSED:
            if (dx || dy) {
                middleBtnState = SCROLLED;
            }
            
            if (middleBtnNotPressed) {
                // Two reports are needed to send the middle button - this is the first
                // The second one below is sent with the button released
                dispatchRelativePointerEvent(dx, dy, MIDDLE_MOUSE_MASK, timestamp);
                middleBtnState = NOT_PRESSED;
            }
            break;
        case SCROLLED:
            if (middleBtnNotPressed) {
                middleBtnState = NOT_PRESSED;
            }
            break;
    }

    buttons &= ~MIDDLE_MOUSE_MASK;
    
    if (middleBtnState == SCROLLED) {
        short scrollY = dy * trackpointScrollMultX / trackpointScrollDivX;
        short scrollX = dx * trackpointScrollMultY / trackpointScrollDivY;
        
        dispatchScrollWheelEvent(scrollY, scrollX, 0, timestamp);
    } else {
        int mulDx = dx * trackpointMultX / trackpointDivX;
        int mulDy = dy * trackpointMultY / trackpointDivY;
        
        dispatchRelativePointerEvent(mulDx, mulDy, buttons, timestamp);
    }
}

void TrackpointDevice::updateRelativePointer(int dx, int dy, int buttons, uint64_t timestamp) {
    dispatchRelativePointerEvent(dx, dy, buttons, timestamp);
};

void TrackpointDevice::updateScrollwheel(short deltaAxis1, short deltaAxis2, short deltaAxis3, uint64_t timestamp) {
    dispatchScrollWheelEvent(deltaAxis1, deltaAxis2, deltaAxis3, timestamp);
}

bool TrackpointDevice::willTerminate(IOService* provider, IOOptionBits options) {
    return  super::willTerminate(provider, options);
}
