#ifndef VOODOO_SIMULATOR_DEVICE_HPP
#define VOODOO_SIMULATOR_DEVICE_HPP

#include <LegacyIOService.h>
#include <LegacyIOHIDDevice.h>

#include <kern/clock.h>

#include "../VoodooInput.hpp"
#include "../VoodooInputMultitouch/VoodooInputTransducer.h"
#include "../VoodooInputMultitouch/VoodooInputEvent.h"
#include "../VoodooInputMultitouch/MultitouchHelpers.h"

#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

#define MT2_MAX_X 7612
#define MT2_MAX_Y 5065

struct __attribute__((__packed__)) MAGIC_TRACKPAD_INPUT_REPORT_FINGER {
    UInt8 AbsX;
    UInt8 AbsXY;
    UInt8 AbsY[2];
    UInt8 Touch_Major;
    UInt8 Touch_Minor;
    UInt8 Size;
    UInt8 Pressure;
    UInt8 Orientation_Origin;
};

struct __attribute__((__packed__)) MAGIC_TRACKPAD_INPUT_REPORT {
    UInt8 ReportID;
    UInt8 Button;
    UInt8 Unused[5];
    
    UInt8 TouchActive;
    
    UInt8 multitouch_report_id;
    UInt8 timestamp_buffer[3];
    
    MAGIC_TRACKPAD_INPUT_REPORT_FINGER FINGERS[VOODOO_INPUT_MAX_TRANSDUCERS]; // May support more fingers
};

class EXPORT VoodooInputSimulatorDevice : public IOHIDDevice {
    OSDeclareDefaultStructors(VoodooInputSimulatorDevice);
    
public:
    void constructReport(const VoodooInputEvent& multitouch_event);

    IOReturn setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) override;

    IOReturn getReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) override;
    IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;
    OSNumber* newVendorIDNumber() const override;
    
    
    OSNumber* newProductIDNumber() const override;
    
    
    OSNumber* newVersionNumber() const override;
    
    
    OSString* newTransportString() const override;
    
    
    OSString* newManufacturerString() const override;
    
    OSNumber* newPrimaryUsageNumber() const override;
    
    OSNumber* newPrimaryUsagePageNumber() const override;
    
    OSString* newProductString() const override;
    
    OSString* newSerialNumberString() const override;
    
    OSNumber* newLocationIDNumber() const override;
    
    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice) override;
    
    bool start(IOService* provider) override;

    void stop(IOService* provider) override;
    
    void releaseResources();

private:
    bool ready_for_reports = false;
    VoodooInput* engine;
    AbsoluteTime start_timestamp;
    OSData* new_get_report_buffer = NULL;
    UInt16 stashed_unknown[15];
    UInt8 touch_state[15];
    UInt8 new_touch_state[15];
    int stylus_check = 0;
    IOWorkLoop* work_loop;
    IOCommandGate* command_gate;
    MAGIC_TRACKPAD_INPUT_REPORT input_report;

    void constructReportGated(const VoodooInputEvent& multitouch_event);
};


#endif
