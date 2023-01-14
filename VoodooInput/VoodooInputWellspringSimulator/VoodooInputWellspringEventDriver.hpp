//
//  VoodooInputWellspringEventDriver.hpp
//  VoodooInput
//
//  Created by Sheika Slate on 1/14/23.
//  Copyright © 2023 Kishor Prins. All rights reserved.
//

#ifndef VoodooInputWellspringEventDriver_hpp
#define VoodooInputWellspringEventDriver_hpp

#include <IOKit/hid/IOHIDEventService.h>

class VoodooInputWellspringSimulator;

class VoodooInputWellspringEventDriver : public IOHIDEventService {
    OSDeclareDefaultStructorsWithDispatch(VoodooInputWellspringEventDriver);
    friend class VoodooInputWellspringSimulator;
public:
    virtual IOReturn setSystemProperties(OSDictionary *) override;
};

#endif /* VoodooInputWellspringEventDriver_hpp */
