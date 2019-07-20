//
//  MultitouchDigitiserTransducer
//
//  Created by Alexandre on 13/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "MultitouchDigitiserTransducer.hpp"

#define super OSObject
OSDefineMetaClassAndStructors(MultitouchDigitiserTransducer, OSObject);

MultitouchDigitiserTransducer* MultitouchDigitiserTransducer::transducer(DigitiserTransducerType transducer_type, IOHIDElement* digitizer_collection) {
    MultitouchDigitiserTransducer* transducer = NULL;
    
    transducer = OSTypeAlloc(MultitouchDigitiserTransducer);
    
    if (!transducer || !transducer->init()) {
        OSSafeReleaseNULL(transducer);
        goto exit;
    }
    
    transducer->type        = transducer_type;
    transducer->collection  = digitizer_collection;
    transducer->in_range    = false;

exit:
    return transducer;
}
