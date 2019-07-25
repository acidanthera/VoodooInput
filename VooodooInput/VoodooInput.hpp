#ifndef VOODOO_INPUT_H
#define VOODOO_INPUT_H

#include <IOKit/IOService.h>

class VoodooInputSimulatorDevice;
class VoodooInputActuatorDevice;

class VoodooInput : public IOService {
    OSDeclareDefaultStructors(VoodooInput);
    
    IOService* parentProvider;
    
    VoodooInputSimulatorDevice* simulator;
    VoodooInputActuatorDevice* actuator;
    
    UInt8 transformKey;
    
    UInt32 logicalMaxX = 0;
    UInt32 logicalMaxY = 0;
    UInt32 physicalMaxX = 0;
    UInt32 physicalMaxY = 0;
public:
    void free() override;

    bool start(IOService* provider) override;
    void stop(IOService* provider) override;
    
    UInt8 getTransformKey();

    UInt32 getPhysicalMaxX();
    UInt32 getPhysicalMaxY();

    UInt32 getLogicalMaxX();
    UInt32 getLogicalMaxY();
    
    IOReturn message(UInt32 type, IOService *provider, void *argument) override;
};

#endif
