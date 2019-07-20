//
//  MultitouchDigitiserTransducer
//
//  Created by Alexandre on 13/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "MultitouchDigitiserTransducer.hpp"

#define super OSObject
OSDefineMetaClassAndStructors(MultitouchDigitiserTransducer, OSObject);

bool MultitouchDigitiserTransducer::serialize(OSSerialize* serializer) {
    OSDictionary* temp_dictionary = OSDictionary::withCapacity(2);

    bool result = false;

    if (temp_dictionary) {
        temp_dictionary->setObject(kIOHIDElementParentCollectionKey, collection);
        temp_dictionary->serialize(serializer);
        temp_dictionary->release();

        result = true;
    }
    
    return result;
}

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
