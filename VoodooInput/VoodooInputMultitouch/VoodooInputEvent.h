//
//  VoodooInputEvent.h
//
//  Copyright © 2019 Kishor Prins. All rights reserved.
//

#ifndef VOODOO_INPUT_EVENT_H
#define VOODOO_INPUT_EVENT_H

#include "VoodooInputTransducer.h"

typedef struct {
    UInt8 contact_count;
    AbsoluteTime timestamp;
    VoodooInputTransducer transducers[VOODOO_INPUT_MAX_TRANSDUCERS];
} VoodooInputEvent;

#endif /* VoodooInputEvent_h */
