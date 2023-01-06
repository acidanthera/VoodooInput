//
//  VoodooInputMT1UserClient.cpp
//  VoodooInput
//
//  Created by Avery Black on 12/31/22.
//  Copyright © 2022 Kishor Prins. All rights reserved.
//

#include "VoodooInputWellspringUserClient.hpp"
#include "VoodooInputWellspringSimulator.hpp"

#define super IOUserClient
OSDefineMetaClassAndStructors(VoodooInputWellspringUserClient, IOUserClient);

#if defined(__x86_64__)
// { Object, Func pointer, Flags, Inputs, Outputs }
static const IOExternalMethod sMethods[VoodooInputMT1UserClientMethodsNumMethods] = {
    // VoodooInputMT1UserClientMethodsSetSendsFrames
    {0, reinterpret_cast<IOMethod>(&VoodooInputWellspringUserClient::sSetSendFrames), kIOUCScalarIScalarO, 1, 0},
    // VoodooInputMT1UserClientMethodsGetReport
    {0, reinterpret_cast<IOMethod>(&VoodooInputWellspringUserClient::sGetReport), kIOUCStructIStructO, sizeof(MTDeviceReportStruct), sizeof(MTDeviceReportStruct)},
    // VoodooInputMT1UserClientMethodsSetReport
    {0, static_cast<IOMethod>(&VoodooInputWellspringUserClient::sNoop), kIOUCStructIStructO, 0x208, 0x208},
    // VoodooInputMT1UserClientMethodsSetSendLogs
    {0, static_cast<IOMethod>(&VoodooInputWellspringUserClient::sNoop), kIOUCScalarIScalarO, 1, 0},
    // VoodooInputMT1UserClientMethodsIssueDriverRequest
    {0, static_cast<IOMethod>(&VoodooInputWellspringUserClient::sNoop), kIOUCStructIStructO, 0x204, 0x204},
    {0, static_cast<IOMethod>(&VoodooInputWellspringUserClient::sNoop), kIOUCScalarIScalarO, 3, 0},    // Relative Mouse Movement
    {0, static_cast<IOMethod>(&VoodooInputWellspringUserClient::sNoop), kIOUCScalarIScalarO, 3, 0},    // Scroll Wheel
    {0, static_cast<IOMethod>(&VoodooInputWellspringUserClient::sNoop), kIOUCScalarIScalarO, 2, 0},    // Keyboard
    {0, static_cast<IOMethod>(&VoodooInputWellspringUserClient::sNoop), kIOUCScalarIScalarO, 1, 0},    // Map Clicks
    // VoodooInputMT1UserClientMethodsRecacheProperties
    {0, static_cast<IOMethod>(&VoodooInputWellspringUserClient::sNoop), kIOUCScalarIScalarO, 0, 0},
    {0, static_cast<IOMethod>(&VoodooInputWellspringUserClient::sNoop), kIOUCScalarIScalarO, 3, 0},    // Momentum Scroll
};

#else // __defined(__x86_64__)
// TODO: Switch where padding is in struct
#error "Invalid architecture"
#endif // __defined(__x86_64__)

bool VoodooInputWellspringUserClient::start(IOService *provider) {
    IOLog("%s Start\n", getName());
    if (!super::start(provider)) return false;
    simulator = OSDynamicCast(VoodooInputWellspringSimulator, provider);
    
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

void VoodooInputWellspringUserClient::stop(IOService *provider) {
    OSSafeReleaseNULL(dataQueue);
    OSSafeReleaseNULL(logQueue);
}

IOReturn VoodooInputWellspringUserClient::registerNotificationPort(mach_port_t port, UInt32 type, UInt32 refCon) {
    IOLog("%s client notif port\n", getName());
    dataQueue->setNotificationPort(port);
    logQueue->setNotificationPort(port);
    return kIOReturnSuccess;
}

IOReturn VoodooInputWellspringUserClient::clientMemoryForType(UInt32 type, IOOptionBits *options, IOMemoryDescriptor **memory) {
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

IOExternalMethod *VoodooInputWellspringUserClient::getTargetAndMethodForIndex(IOService **targetP, UInt32 index) {
    IOLog("%s External Method %d\n", getName(), index);
    if (index >= VoodooInputMT1UserClientMethodsNumMethods) {
        return nullptr;
    }
    
    *targetP = this;
    return const_cast<IOExternalMethod *>(&sMethods[index]);
}

IOReturn VoodooInputWellspringUserClient::sSetSendFrames(bool enableReports) {
    bool success = true;
    IOLog("%s Set Send Frames: %d\n", getName(), enableReports);
    
    if (enableReports) {
        success = simulator->registerUserClient(this);
    } else {
        simulator->unregisterUserClient(this);
    }
    
    return success ? kIOReturnSuccess : kIOReturnError;
}

// I'm not really sure why they use two different structs here???
IOReturn VoodooInputWellspringUserClient::sGetReport(MTDeviceReportStruct *input, MTDeviceReportStruct *output) {
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

void VoodooInputWellspringUserClient::enqueueData(void *data, size_t size) {
    if (dataQueue == nullptr) return;
    IOLog("%s Enqueue Data\n", getName());
    dataQueue->enqueue(data, (UInt32) size);
}

IOReturn VoodooInputWellspringUserClient::sNoop(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    return kIOReturnSuccess; // noop
}
