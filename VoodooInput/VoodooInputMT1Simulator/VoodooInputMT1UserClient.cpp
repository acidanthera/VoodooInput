//
//  VoodooInputMT1UserClient.cpp
//  VoodooInput
//
//  Created by Avery Black on 12/31/22.
//  Copyright © 2022 Kishor Prins. All rights reserved.
//

#include "VoodooInputMT1UserClient.hpp"

#define super IOUserClient
OSDefineMetaClassAndStructors(VoodooInputMT1UserClient, IOUserClient);

#if defined(__x86_64__)
// { Object, Func pointer, Padding, Flags, Inputs, Outputs }
static const IOExternalMethod sMethods[VoodooInputMT1UserClientMethodsNumMethods] = {
    // VoodooInputMT1UserClientMethodsSetSendsFrames
    {0, static_cast<IOMethod>(&VoodooInputMT1UserClient::sSetSendFrames), kIOUCScalarIScalarO, 1, 0},
    // VoodooInputMT1UserClientMethodsGetReport
    {0, static_cast<IOMethod>(&VoodooInputMT1UserClient::sGetReport), kIOUCStructIStructO, 0x208, 0x208},
    // VoodooInputMT1UserClientMethodsSetReport
    {0, static_cast<IOMethod>(&VoodooInputMT1UserClient::sNoop), kIOUCStructIStructO, 0x208, 0x208},
    // VoodooInputMT1UserClientMethodsSetSendLogs
    {0, static_cast<IOMethod>(&VoodooInputMT1UserClient::sNoop), kIOUCScalarIScalarO, 1, 0},
    // VoodooInputMT1UserClientMethodsIssueDriverRequest
    {0, static_cast<IOMethod>(&VoodooInputMT1UserClient::sNoop), kIOUCStructIStructO, 0x204, 0x204},
    {0, static_cast<IOMethod>(&VoodooInputMT1UserClient::sNoop), kIOUCScalarIScalarO, 3, 0},    // Relative Mouse Movement
    {0, static_cast<IOMethod>(&VoodooInputMT1UserClient::sNoop), kIOUCScalarIScalarO, 3, 0},    // Scroll Wheel
    {0, static_cast<IOMethod>(&VoodooInputMT1UserClient::sNoop), kIOUCScalarIScalarO, 2, 0},    // Keyboard
    {0, static_cast<IOMethod>(&VoodooInputMT1UserClient::sNoop), kIOUCScalarIScalarO, 1, 0},    // Map Clicks
    // VoodooInputMT1UserClientMethodsRecacheProperties
    {0, static_cast<IOMethod>(&VoodooInputMT1UserClient::sNoop), kIOUCScalarIScalarO, 0, 0},
    {0, static_cast<IOMethod>(&VoodooInputMT1UserClient::sNoop), kIOUCScalarIScalarO, 3, 0},    // Momentum Scroll
};

#else // __defined(__x86_64__)
// TODO: Switch where padding is in struct
#error "Invalid architecture"
#endif // __defined(__x86_64__)

bool VoodooInputMT1UserClient::start(IOService *provider) {
    IOLog("%s Start\n", getName());
    if (!super::start(provider)) return false;
    simulator = OSDynamicCast(VoodooInputMT1Simulator, provider);
    
    if (simulator == nullptr) {
        IOLog("%s Invalid provider!\n", getName());
        return false;
    }
    
    dataQueue = IOSharedDataQueue::withCapacity(0x10004);
    // Don't think we need to use this? Setting a small size for now
    logQueue = IOSharedDataQueue::withCapacity(0x1);
    
    if (dataQueue == nullptr || logQueue == nullptr) {
        return false;
    }
    
    dataQueueDesc = dataQueue->getMemoryDescriptor();
    logQueueDesc = logQueue->getMemoryDescriptor();
    if (dataQueueDesc == nullptr || logQueueDesc == nullptr) {
        return false;
    }
    
    return true;
}

void VoodooInputMT1UserClient::stop(IOService *provider) {
    OSSafeReleaseNULL(dataQueue);
    OSSafeReleaseNULL(logQueue);
}

IOReturn VoodooInputMT1UserClient::registerNotificationPort(mach_port_t port, UInt32 type, UInt32 refCon) {
    IOLog("%s client notif port\n", getName());
    dataQueue->setNotificationPort(port);
    logQueue->setNotificationPort(port);
    return kIOReturnSuccess;
}

IOReturn VoodooInputMT1UserClient::clientMemoryForType(UInt32 type, IOOptionBits *options, IOMemoryDescriptor **memory) {
    IOLog("%s clientMem\n", getName());
    if (type != 0x10) {
        dataQueueDesc->retain();
        *memory = dataQueueDesc;
    } else {
        logQueueDesc->retain();
        *memory = logQueue->getMemoryDescriptor();
    }
    
    *options = 0;
    return kIOReturnSuccess;
}

IOExternalMethod *VoodooInputMT1UserClient::getTargetAndMethodForIndex(IOService **targetP, UInt32 index) {
    IOLog("%s External Method %d\n", getName(), index);
    if (index >= VoodooInputMT1UserClientMethodsNumMethods) {
        return nullptr;
    }
    
    *targetP = this;
    return const_cast<IOExternalMethod *>(&sMethods[index]);
}

IOReturn VoodooInputMT1UserClient::sSetSendFrames(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    bool success = true;
    IOLog("%s Get Report: %d\n", getName(), static_cast<bool>(p1));
    
    if (static_cast<bool>(p1)) {
        success = simulator->registerUserClient(this);
    } else {
        simulator->unregisterUserClient(this);
    }
    
    return success ? kIOReturnSuccess : kIOReturnError;
}

// I'm not really sure why they use two different structs here???
IOReturn VoodooInputMT1UserClient::sGetReport(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    MT1DeviceReportStruct *input = static_cast<MT1DeviceReportStruct *>(p1);
    MT1DeviceReportStruct *output = static_cast<MT1DeviceReportStruct *>(p2);
    if (input == nullptr || output == nullptr) {
        return kIOReturnBadArgument;
    }
    
    IOLog("%s Get Report: %d\n", getName(), input->reportId);
    
    IOReturn ret = simulator->getReport(input);
    if (ret == kIOReturnSuccess) {
        memmove(output->data, input->data, input->dataSize);
        output->dataSize = input->dataSize;
    }
    
    return ret;
}

void VoodooInputMT1UserClient::enqueueData(void *data, UInt32 size) {
    if (dataQueue == nullptr) return;
    IOLog("%s Enqueue Data\n", getName());
    dataQueue->enqueue(data, size);
}

IOReturn VoodooInputMT1UserClient::sNoop(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    return kIOReturnSuccess; // noop
}
