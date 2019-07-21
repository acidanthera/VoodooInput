//
//  VoodooInputMessages.h
//  VooodooInput
//
//  Created by Kishor Prins on 2019-07-20.
//  Copyright © 2019 Kishor Prins. All rights reserved.
//

#ifndef VOODOO_INPUT_MESSAGES_H
#define VOODOO_INPUT_MESSAGES_H

#define VOODOO_INPUT_TRANSFORM_KEY "IOFBTransform"
#define VOODOO_INPUT_LOGICAL_MAX_X_KEY "Logical Max X"
#define VOODOO_INPUT_LOGICAL_MAX_Y_KEY "Logical Max Y"
#define VOODOO_INPUT_PHYSICAL_MAX_X_KEY "Physical Max X"
#define VOODOO_INPUT_PHYSICAL_MAX_Y_KEY "Physical Max Y"

#define VOODOO_INPUT_MAX_TRANSDUCERS 10
#define kIOMessageVoodooInputMessage 0x0123

#define kVoodooInputTransducerFingerType 1
#define kVoodooInputTransducerStylusType 2

struct VoodooInputTransducer {
    int id;
    int secondary_id;
    
    int type;

    bool valid;
    bool tipswitch;
    bool physicalButton;
    
    UInt32 xValue;
    UInt32 previousXValue;
    
    UInt32 yValue;
    UInt32 previousYValue;
    
    UInt32 pressure;
    UInt32 maxPressureLevel;
};

struct VoodooInputMessage {
    UInt64 timestamp;
    VoodooInputTransducer transducers[VOODOO_INPUT_MAX_TRANSDUCERS];
    int numTransducers;
    
};

#endif /* VoodooInputMessages_h */
