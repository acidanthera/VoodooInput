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
    setProperty(kIOHIDScrollResolutionKey, 800 << 16, 32);
    setProperty("HIDScrollResolutionX", 800 << 16, 32);
    setProperty("HIDScrollResolutionY", 800 << 16, 32);

    registerService();
    return true;
}

void TrackpointDevice::updateTrackpointProperties() {
    OSDictionary *dict = OSDynamicCast(OSDictionary, getProperty(VOODOO_TRACKPOINT_KEY));
    
    if (dict == nullptr) return;
    
    OSNumber *btns = OSDynamicCast(OSNumber, dict->getObject(VOODOO_TRACKPOINT_BTN_CNT));
    OSNumber *deadzone = OSDynamicCast(OSNumber, dict->getObject(VOODOO_TRACKPOINT_DEADZONE));
    OSNumber *mouseMultX = OSDynamicCast(OSNumber, dict->getObject(VOODOO_TRACKPOINT_MOUSE_MULT_X));
    OSNumber *mouseMultY = OSDynamicCast(OSNumber, dict->getObject(VOODOO_TRACKPOINT_MOUSE_MULT_Y));
    OSNumber *scrollMultX = OSDynamicCast(OSNumber, dict->getObject(VOODOO_TRACKPOINT_SCROLL_MULT_X));
    OSNumber *scrollMultY = OSDynamicCast(OSNumber, dict->getObject(VOODOO_TRACKPOINT_SCROLL_MULT_Y));
    
    if (btns != nullptr) btnCount = btns->unsigned32BitValue();
    if (deadzone != nullptr) trackpointDeadzone = deadzone->unsigned32BitValue();
    if (mouseMultX != nullptr) trackpointMultX = mouseMultX->unsigned32BitValue();
    if (mouseMultY != nullptr) trackpointMultY = mouseMultY->unsigned32BitValue();
    if (scrollMultX != nullptr) trackpointScrollMultX = scrollMultX->unsigned32BitValue();
    if (scrollMultY != nullptr) trackpointScrollMultY = scrollMultY->unsigned32BitValue();
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

    // Must multiply first then divide so we don't multiply by zero
    if (middleBtnState == SCROLLED) {
        SInt32 scrollY = -dy * trackpointScrollMultX / DEFAULT_MULT;
        SInt32 scrollX = -dx * trackpointScrollMultY / DEFAULT_MULT;
        
        dispatchScrollWheelEvent(scrollY, scrollX, 0, timestamp);
    } else {
        SInt32 mulDx = dx * trackpointMultX / DEFAULT_MULT;
        SInt32 mulDy = dy * trackpointMultY / DEFAULT_MULT;
        
        dispatchRelativePointerEvent(mulDx, mulDy, buttons, timestamp);
    }
}

void TrackpointDevice::updateRelativePointer(int dx, int dy, int buttons, uint64_t timestamp) {
    dispatchRelativePointerEvent(dx, dy, buttons, timestamp);
};

void TrackpointDevice::updateScrollwheel(short deltaAxis1, short deltaAxis2, short deltaAxis3, uint64_t timestamp) {
    dispatchScrollWheelEvent(deltaAxis1, deltaAxis2, deltaAxis3, timestamp);
}
