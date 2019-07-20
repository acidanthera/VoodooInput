#ifndef VOODOO_INPUT_H
#define VOODOO_INPUT_H

#include <IOKit/IOService.h>

#include "Native Simulator/VoodooActuatorDevice.hpp"
#include "Native Simulator/VoodooSimulatorDevice.hpp"

class VoodooInput : public IOService {
    OSDeclareDefaultStructors(VoodooInput);
    
    IOService* parentProvider;
    
    VoodooSimulatorDevice* simulator;
    VoodooActuatorDevice* actuator;
public:
    bool init(OSDictionary* properties) override;
    void free() override;

    bool start(IOService* provider) override;
    void stop(IOService* device) override;
    
    IOReturn message(UInt32 type, IOService *provider, void *argument) override;
};

#endif
