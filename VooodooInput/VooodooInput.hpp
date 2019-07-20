#ifndef VOODOO_INPUT_H
#define VOODOO_INPUT_H

#include <IOKit/IOService.h>

class VoodooInput : public IOService {
    OSDeclareDefaultStructors(VoodooInput);
    
    IOService* provider;
public:
    bool start(IOService* provider) override;
    void stop(IOService* device) override;
    
    IOReturn message(UInt32 type, IOService *provider, void *argument) override;
};

#endif
