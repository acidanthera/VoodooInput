//
//  VoodooInputSimulatorDevice.cpp
//  VoodooI2C
//
//  Created by Alexandre on 10/02/2018.
//  Copyright © 2018 Alexandre Daoud and Kishor Prins. All rights reserved.
//

#include "VoodooInput.hpp"
#include "VoodooInputSimulatorDevice.hpp"
#include "../VoodooInputMultitouch/MultitouchHelpers.h"

#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>

#define super IOHIDDevice
OSDefineMetaClassAndStructors(VoodooInputSimulatorDevice, IOHIDDevice);

UInt16 abs(SInt16 x) {
    if (x < 0)
        return x * -1;
    return x;
}

const unsigned char report_descriptor[] = {0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x09, 0x01, 0xa1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x85, 0x02, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05, 0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x02, 0x81, 0x06, 0x95, 0x04, 0x75, 0x08, 0x81, 0x01, 0xc0, 0xc0, 0x05, 0x0d, 0x09, 0x05, 0xa1, 0x01, 0x06, 0x00, 0xff, 0x09, 0x0c, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95, 0x10, 0x85, 0x3f, 0x81, 0x22, 0xc0, 0x06, 0x00, 0xff, 0x09, 0x0c, 0xa1, 0x01, 0x06, 0x00, 0xff, 0x09, 0x0c, 0x15, 0x00, 0x26, 0xff, 0x00, 0x85, 0x44, 0x75, 0x08, 0x96, 0x6b, 0x05, 0x81, 0x00, 0xc0};

void VoodooInputSimulatorDevice::constructReport(const VoodooInputEvent& multitouch_event) {
    if (!ready_for_reports)
        return;

    command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &VoodooInputSimulatorDevice::constructReportGated), (void*)&multitouch_event);
}

void VoodooInputSimulatorDevice::constructReportGated(const VoodooInputEvent& multitouch_event) {
    if (!ready_for_reports)
        return;
    
    AbsoluteTime timestamp = multitouch_event.timestamp;

    input_report.ReportID = 0x02;
    input_report.Unused[0] = 0;
    input_report.Unused[1] = 0;
    input_report.Unused[2] = 0;
    input_report.Unused[3] = 0;
    input_report.Unused[4] = 0;
    
    const VoodooInputTransducer* transducer = &multitouch_event.transducers[0];
    
    if (transducer->isValid && transducer->type == VoodooInputTransducerType::STYLUS)
        stylus_check = 1;
    
    // physical button
    input_report.Button = transducer->isPhysicalButtonDown;
    
    // touch active

    // rotation check
    
    UInt8 transform = engine->getTransformKey();

    // multitouch report id
    input_report.multitouch_report_id = 0x31; // Magic
    
    // timestamp
    AbsoluteTime relative_timestamp = timestamp;
    
    SUB_ABSOLUTETIME(&relative_timestamp, &start_timestamp);
    
    UInt64 milli_timestamp;
    
    absolutetime_to_nanoseconds(relative_timestamp, &milli_timestamp);
    
    milli_timestamp /= 1000000;
    
    input_report.timestamp_buffer[0] = (milli_timestamp << 0x3) | 0x4;
    input_report.timestamp_buffer[1] = (milli_timestamp >> 0x5) & 0xFF;
    input_report.timestamp_buffer[2] = (milli_timestamp >> 0xd) & 0xFF;
    
    // finger data
    bool input_active = input_report.Button;
    bool is_error_input_active = false;
    
    for (int i = 0; i < multitouch_event.contact_count + 1; i++) {
        const VoodooInputTransducer* transducer = &multitouch_event.transducers[i + stylus_check];

        if (!transducer || !transducer->isValid)
            continue;

        if (transducer->type == VoodooInputTransducerType::STYLUS) {
            continue;
        }

        // in case the obtained id is greater than 14, usually 0~4 for common devices.
        UInt16 finger_id = transducer->secondaryId % 15;
        if (!transducer->isTransducerActive) {
            touch_state[finger_id] = 0;
        } else {
            input_active = true;
        }

        MAGIC_TRACKPAD_INPUT_REPORT_FINGER& finger_data = input_report.FINGERS[i];
        
        SInt16 x_min = MT2_MAX_X / 2;
        SInt16 y_min = MT2_MAX_Y / 2;
        
        IOFixed scaled_x = ((transducer->currentCoordinates.x * 1.0f) / engine->getLogicalMaxX()) * MT2_MAX_X;
        IOFixed scaled_y = ((transducer->currentCoordinates.y * 1.0f) / engine->getLogicalMaxY()) * MT2_MAX_Y;

        if (scaled_x < 1 && scaled_y >= MT2_MAX_Y) {
            is_error_input_active = true;
        }
        
        IOFixed scaled_old_x = ((transducer->previousCoordinates.x * 1.0f) / engine->getLogicalMaxX()) * MT2_MAX_X;
        IOFixed scaled_old_y = ((transducer->previousCoordinates.y * 1.0f) / engine->getLogicalMaxY()) * MT2_MAX_Y;

        
        if (transform) {
            if (transform & kIOFBSwapAxes) {
                scaled_x = ((transducer->currentCoordinates.y * 1.0f) / engine->getLogicalMaxY()) * MT2_MAX_X;
                scaled_y = ((transducer->currentCoordinates.x * 1.0f) / engine->getLogicalMaxX()) * MT2_MAX_Y;
            }
            
            if (transform & kIOFBInvertX) {
                scaled_x = MT2_MAX_X - scaled_x;
            }
            if (transform & kIOFBInvertY) {
                scaled_y = MT2_MAX_Y - scaled_y;
            }
        }

        UInt8 angle_bits = 0;
        IOFixed x_diff = scaled_x - scaled_old_x;
        IOFixed y_diff = (scaled_y - scaled_old_y) * -1;
        if (x_diff > 0) {
            if (y_diff <= -5.027f * x_diff) {
                // if: y_diff/x_diff <= tan(pi/2 + pi/8 * 1/2) < 0
                // then: angle ~= (pi/2), angle_bits = (pi/2) / pi * 8
                angle_bits = 0x4;
            } else if (y_diff <= -1.497f * x_diff) {
                // if: y_diff/x_diff <= tan(5pi/8 + pi/8 * 1/2) < 0
                // then: angle ~= (5pi/8), angle_bits = (5pi/8) / pi * 8
                angle_bits = 0x5;
            } else if (y_diff <= -0.668f * x_diff) {
                angle_bits = 0x6;
            } else if (y_diff <= -0.199f * x_diff) {
                angle_bits = 0x7;
            } else if (y_diff <= 0.199f * x_diff) {
                angle_bits = 0x0;
            } else if (y_diff <= 0.668f * x_diff) {
                angle_bits = 0x1;
            } else if (y_diff <= 1.497f * x_diff) {
                angle_bits = 0x2;
            } else if (y_diff <= 5.027f * x_diff) {
                angle_bits = 0x3;
            } else {
                angle_bits = 0x4;
            }
        } else {
            if (y_diff >= -5.027f * x_diff) {
                angle_bits = 0x4;
            } else if (y_diff >= -1.497f * x_diff) {
                angle_bits = 0x5;
            } else if (y_diff >= -0.668f * x_diff) {
                angle_bits = 0x6;
            } else if (y_diff >= -0.199f * x_diff) {
                angle_bits = 0x7;
            } else if (y_diff >= 0.199f * x_diff) {
                angle_bits = 0x0;
            } else if (y_diff >= 0.668f * x_diff) {
                angle_bits = 0x1;
            } else if (y_diff >= 1.497f * x_diff) {
                angle_bits = 0x2;
            } else if (y_diff >= 5.027f * x_diff) {
                angle_bits = 0x3;
            } else {
                angle_bits = 0x4;
            }
        }
        
        touch_state[finger_id]++;
        if (touch_state[finger_id] > 4) {
            finger_data.State = 0x4;
        } else {
            finger_data.State = touch_state[finger_id];
        }
        
        finger_data.Priority = 4 - i;

        if (transducer->supportsPressure) {
            finger_data.Pressure = transducer->currentCoordinates.pressure;
            finger_data.Size = transducer->currentCoordinates.width;
            finger_data.Touch_Major = transducer->currentCoordinates.width;
            finger_data.Touch_Minor = transducer->currentCoordinates.width;
        } else {
            finger_data.Size = 10;
            finger_data.Touch_Minor = 20;
            finger_data.Touch_Major = 20;
        }
        
        if (input_report.Button) {
            finger_data.Pressure = 120;
        }
        
        if (!transducer->isTransducerActive && !input_report.Button) {
            finger_data.State = 0x7;
            finger_data.Priority = 0x5;
            finger_data.Size = 0x0;
            finger_data.Pressure = 0x0;
            finger_data.Touch_Minor = 0;
            finger_data.Touch_Major = 0;
        }

        finger_data.X = (SInt16)(scaled_x - x_min);
        finger_data.Y = (SInt16)(scaled_y - y_min) * -1;
        
        finger_data.Angle = angle_bits;
        finger_data.Identifier = finger_id + 1;
    }

    if (input_active)
        input_report.TouchActive = 0x3;
    else
        input_report.TouchActive = 0x2;
    
    int total_report_len = (9 * multitouch_event.contact_count) + 12;
    IOBufferMemoryDescriptor* buffer_report = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, total_report_len);

    if (!is_error_input_active) {
      buffer_report->writeBytes(0, &input_report, total_report_len);
      handleReport(buffer_report, kIOHIDReportTypeInput);
    }

    buffer_report->release();
    buffer_report = NULL;

    if (!input_active) {
        for (int i = 0; i < 15; i++) {
            touch_state[i] = 0;
        }

        input_report.FINGERS[0].Size = 0x0;
        input_report.FINGERS[0].Pressure = 0x0;
        input_report.FINGERS[0].Touch_Major = 0x0;
        input_report.FINGERS[0].Touch_Minor = 0x0;

        milli_timestamp += 10;
        
        input_report.timestamp_buffer[0] = (milli_timestamp << 0x3) | 0x4;
        input_report.timestamp_buffer[1] = (milli_timestamp >> 0x5) & 0xFF;
        input_report.timestamp_buffer[2] = (milli_timestamp >> 0xd) & 0xFF;
        
        buffer_report = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, total_report_len);
        buffer_report->writeBytes(0, &input_report, total_report_len);
        handleReport(buffer_report, kIOHIDReportTypeInput);
        buffer_report->release();
        
        input_report.FINGERS[0].Priority = 0x5;
        input_report.FINGERS[0].State = 0x0;
        
        buffer_report = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, total_report_len);
        buffer_report->writeBytes(0, &input_report, total_report_len);
        handleReport(buffer_report, kIOHIDReportTypeInput);
        buffer_report->release();
        
        milli_timestamp += 10;
        
        input_report.timestamp_buffer[0] = (milli_timestamp << 0x3) | 0x4;
        input_report.timestamp_buffer[1] = (milli_timestamp >> 0x5) & 0xFF;
        input_report.timestamp_buffer[2] = (milli_timestamp >> 0xd) & 0xFF;

        buffer_report = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, 12);
        buffer_report->writeBytes(0, &input_report, 12);
        handleReport(buffer_report, kIOHIDReportTypeInput);
        buffer_report->release();
    }
    
    input_report = {};
}

bool VoodooInputSimulatorDevice::start(IOService* provider) {
    if (!super::start(provider))
        return false;
    
    ready_for_reports = false;
    
    clock_get_uptime(&start_timestamp);
    
    engine = OSDynamicCast(VoodooInput, provider);
    
    if (!engine)
        return false;

    work_loop = this->getWorkLoop();
    if (!work_loop) {
        IOLog("%s Could not get a IOWorkLoop instance\n", getName());
        releaseResources();
        return false;
    }
    
    work_loop->retain();
    
    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate || (work_loop->addEventSource(command_gate) != kIOReturnSuccess)) {
        IOLog("%s Could not open command gate\n", getName());
        releaseResources();
        return false;
    }

    PMinit();
    provider->joinPMtree(this);
    registerPowerDriver(this, PMPowerStates, kIOPMNumberPowerStates);

    for (int i = 0; i < 15; i++) {
        touch_state[i] = 0;
    }
    
    stylus_check = 0;
    
    new_get_report_buffer = nullptr;
    
    ready_for_reports = true;
    
    return true;
}

void VoodooInputSimulatorDevice::stop(IOService* provider) {
    releaseResources();
    
    PMstop();
    
    super::stop(provider);
}

IOReturn VoodooInputSimulatorDevice::setPowerState(unsigned long whichState, IOService* whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        // ready_for_reports = false;
    } else {
        // ready_for_reports = true;
    }
    return kIOPMAckImplied;
}

void VoodooInputSimulatorDevice::releaseResources() {
    if (command_gate) {
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }
    
    OSSafeReleaseNULL(work_loop);
    
    OSSafeReleaseNULL(new_get_report_buffer);
}

IOReturn VoodooInputSimulatorDevice::setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) {
    UInt32 report_id = options & 0xFF;
    
    if (report_id == 0x1) {
        char* raw_buffer = (char*)IOMalloc(report->getLength());
        
        report->prepare();
        
        report->readBytes(0, raw_buffer, report->getLength());
        
        report->complete();
        
        if (new_get_report_buffer) {
            new_get_report_buffer->release();
        }
        
        new_get_report_buffer = OSData::withCapacity(1);
        
        if (!new_get_report_buffer) {
            return kIOReturnNoResources;
        }
        
        UInt8 value = raw_buffer[1];
        
        if (value == 0xDB) {
            unsigned char buffer[] = {0x1, 0xDB, 0x00, 0x49, 0x00};
            new_get_report_buffer->appendBytes(buffer, sizeof(buffer));
        }
        
        if (value == 0xD1) {
            unsigned char buffer[] = {0x1, 0xD1, 0x00, 0x01, 0x00};
            new_get_report_buffer->appendBytes(buffer, sizeof(buffer));
        }
        
        if (value == 0xD3) {
            unsigned char buffer[] = {0x1, 0xD3, 0x00, 0x0C, 0x00};
            new_get_report_buffer->appendBytes(buffer, sizeof(buffer));
        }
        
        if (value == 0xD0) {
            unsigned char buffer[] = {0x1, 0xD0, 0x00, 0x0F, 0x00};
            new_get_report_buffer->appendBytes(buffer, sizeof(buffer));
        }
        
        if (value == 0xA1) {
            unsigned char buffer[] = {0x1, 0xA1, 0x00, 0x06, 0x00};
            new_get_report_buffer->appendBytes(buffer, sizeof(buffer));
        }
        
        if (value == 0xD9) {
            //Sensor Surface Width = 0x3cf0 (0xf0, 0x3c) = 15.600 cm
            //Sensor Surface Height = 0x2b20 (0x20, 0x2b) = 11.040 cm

            // It's already in 0.01 mm units
            uint32_t rawWidth = engine->getPhysicalMaxX();
            uint32_t rawHeight = engine->getPhysicalMaxY();

            uint8_t rawWidthLower = rawWidth & 0xff;
            uint8_t rawWidthHigher = (rawWidth >> 8) & 0xff;

            uint8_t rawHeightLower = rawHeight & 0xff;
            uint8_t rawHeightHigher = (rawHeight >> 8) & 0xff;

            unsigned char buffer[] = {0xD9, rawWidthLower, rawWidthHigher, 0x00, 0x00, rawHeightLower, rawHeightHigher, 0x00, 0x00, 0x44, 0xE3, 0x52, 0xFF, 0xBD, 0x1E, 0xE4, 0x26}; //Sensor Surface Description
            new_get_report_buffer->appendBytes(buffer, sizeof(buffer));
        }
        
        if (value == 0x7F) {
            unsigned char buffer[] = {0x1, 0x7F, 0x00, 0x04, 0x00};
            new_get_report_buffer->appendBytes(buffer, sizeof(buffer));
        }
        
        if (value == 0xC8) {
            unsigned char buffer[] = {0x1, 0xC8, 0x00, 0x01, 0x00};
            new_get_report_buffer->appendBytes(buffer, sizeof(buffer));
        }
        
        IOFree(raw_buffer, report->getLength());
    }
    
    return kIOReturnSuccess;
}

IOReturn VoodooInputSimulatorDevice::getReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) {
    UInt32 report_id = options & 0xFF;
    Boolean getReportedAllocated = true;
    
    OSData* get_buffer = OSData::withCapacity(1);

    if (!get_buffer || (report_id == 0x1 && !new_get_report_buffer)) {
       OSSafeReleaseNULL(get_buffer);
       return kIOReturnNoResources;
    }
    
    if (report_id == 0x0) {
        unsigned char buffer[] = {0x0, 0x01};
        get_buffer->appendBytes(buffer, sizeof(buffer));
    }
    
    if (report_id == 0x1) {
        get_buffer->release();
        get_buffer = new_get_report_buffer;
        getReportedAllocated = false;
    }
    
    if (report_id == 0xD1) {
        unsigned char buffer[] = {0xD1, 0x81};
        // Family ID = 0x81
        get_buffer->appendBytes(buffer, sizeof(buffer));
    }
    
    if (report_id == 0xD3) {
        unsigned char buffer[] = {0xD3, 0x01, 0x16, 0x1E, 0x03, 0x95, 0x00, 0x14, 0x1E, 0x62, 0x05, 0x00, 0x00};
        // Sensor Rows = 0x16
        // Sensor Columns = 0x1e
        get_buffer->appendBytes(buffer, sizeof(buffer));
    }
    
    if (report_id == 0xD0) {
        unsigned char buffer[] = {0xD0, 0x02, 0x01, 0x00, 0x14, 0x01, 0x00, 0x1E, 0x00, 0x02, 0x14, 0x02, 0x01, 0x0E, 0x02, 0x00}; // Sensor Region Description
        get_buffer->appendBytes(buffer, sizeof(buffer));
    }
    
    if (report_id == 0xA1) {
        unsigned char buffer[] = {0xA1, 0x00, 0x00, 0x05, 0x00, 0xFC, 0x01}; // Sensor Region Param
        get_buffer->appendBytes(buffer, sizeof(buffer));
    }
    
    if (report_id == 0xD9) {
        // Sensor Surface Width = 0x3cf0 (0xf0, 0x3c) = 15.600 cm
        // Sensor Surface Height = 0x2b20 (0x20, 0x2b) = 11.040 cm
        
        uint32_t rawWidth = engine->getPhysicalMaxX() * 10;
        uint32_t rawHeight = engine->getPhysicalMaxY() * 10;
        
        uint8_t rawWidthLower = rawWidth & 0xff;
        uint8_t rawWidthHigher = (rawWidth >> 8) & 0xff;
        
        uint8_t rawHeightLower = rawHeight & 0xff;
        uint8_t rawHeightHigher = (rawHeight >> 8) & 0xff;
        
        unsigned char buffer[] = {0xD9, rawWidthLower, rawWidthHigher, 0x00, 0x00, rawHeightLower, rawHeightHigher, 0x00, 0x00, 0x44, 0xE3, 0x52, 0xFF, 0xBD, 0x1E, 0xE4, 0x26}; // Sensor Surface Description
        get_buffer->appendBytes(buffer, sizeof(buffer));
    }
    
    if (report_id == 0x7F) {
        unsigned char buffer[] = {0x7F, 0x00, 0x00, 0x00, 0x00};
        get_buffer->appendBytes(buffer, sizeof(buffer));
    }
    
    if (report_id == 0xC8) {
        unsigned char buffer[] = {0xC8, 0x08};
        get_buffer->appendBytes(buffer, sizeof(buffer));
    }
    
    if (report_id == 0x2) {
        unsigned char buffer[] = {0x02, 0x01};
        get_buffer->appendBytes(buffer, sizeof(buffer));
    }
    
    if (report_id == 0xDB) {
        uint32_t rawWidth = engine->getPhysicalMaxX();
        uint32_t rawHeight = engine->getPhysicalMaxY();
        
        uint8_t rawWidthLower = rawWidth & 0xff;
        uint8_t rawWidthHigher = (rawWidth >> 8) & 0xff;
        
        uint8_t rawHeightLower = rawHeight & 0xff;
        uint8_t rawHeightHigher = (rawHeight >> 8) & 0xff;
        
        unsigned char buffer[] = {0xDB, 0x01, 0x02, 0x00,
            /* Start 0xD1 */ 0xD1, 0x81, /* End 0xD1 */
            0x0D, 0x00,
            /* Start 0xD3 */ 0xD3, 0x01, 0x16, 0x1E, 0x03, 0x95, 0x00, 0x14, 0x1E, 0x62, 0x05, 0x00, 0x00, /* End 0xD3 */
            0x10, 0x00,
            /* Start 0xD0 */ 0xD0, 0x02, 0x01, 0x00, 0x14, 0x01, 0x00, 0x1E, 0x00, 0x02, 0x14, 0x02, 0x01, 0x0E, 0x02, 0x00, /* End 0xD0 */
            0x07, 0x00,
            /* Start 0xA1 */ 0xA1, 0x00, 0x00, 0x05, 0x00, 0xFC, 0x01, /* End 0xA1 */
            0x11, 0x00,
            /* Start 0xD9 */ 0xD9, rawWidthLower, rawWidthHigher, 0x00, 0x00, rawHeightLower, rawHeightHigher, 0x00, 0x00, 0x44, 0xE3, 0x52, 0xFF, 0xBD, 0x1E, 0xE4, 0x26,
            /* Start 0x7F */ 0x7F, 0x00, 0x00, 0x00, 0x00 /*End 0x7F */};
        get_buffer->appendBytes(buffer, sizeof(buffer));
    }
    
    report->writeBytes(0, get_buffer->getBytesNoCopy(), get_buffer->getLength());
    
   if (getReportedAllocated) {
        get_buffer->release();
   }
    
    return kIOReturnSuccess;
}

IOReturn VoodooInputSimulatorDevice::newReportDescriptor(IOMemoryDescriptor** descriptor) const {
    IOBufferMemoryDescriptor* report_descriptor_buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof(report_descriptor));
    
    if (!report_descriptor_buffer) {
        IOLog("%s Could not allocate buffer for report descriptor\n", getName());
        return kIOReturnNoResources;
    }
    
    report_descriptor_buffer->writeBytes(0, report_descriptor, sizeof(report_descriptor));
    *descriptor = report_descriptor_buffer;
    
    return kIOReturnSuccess;
}

OSString* VoodooInputSimulatorDevice::newManufacturerString() const {
    return OSString::withCString("Apple Inc.");
}

OSNumber* VoodooInputSimulatorDevice::newPrimaryUsageNumber() const {
    return OSNumber::withNumber(kHIDUsage_GD_Mouse, 32);
}

OSNumber* VoodooInputSimulatorDevice::newPrimaryUsagePageNumber() const {
    return OSNumber::withNumber(kHIDPage_GenericDesktop, 32);
}

OSNumber* VoodooInputSimulatorDevice::newProductIDNumber() const {
    return OSNumber::withNumber(0x272, 32);
}

OSString* VoodooInputSimulatorDevice::newProductString() const {
    return OSString::withCString("Magic Trackpad 2");
}

OSString* VoodooInputSimulatorDevice::newSerialNumberString() const {
    return OSString::withCString("Voodoo Magic Trackpad 2 Simulator");
}

OSString* VoodooInputSimulatorDevice::newTransportString() const {
    return OSString::withCString("I2C");
}

OSNumber* VoodooInputSimulatorDevice::newVendorIDNumber() const {
    return OSNumber::withNumber(0x5ac, 16);
}

OSNumber* VoodooInputSimulatorDevice::newLocationIDNumber() const {
    return OSNumber::withNumber(0x14400000, 32);
}

OSNumber* VoodooInputSimulatorDevice::newVersionNumber() const {
    return OSNumber::withNumber(0x804, 32);
}
