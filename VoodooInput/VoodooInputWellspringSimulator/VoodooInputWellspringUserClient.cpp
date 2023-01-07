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
#define MTExternalMethod(method, flags, inputs, outputs) {0, method, kIOExternalMethodACIDPadding, flags, inputs, outputs}
#elif defined(__i386__)
#define MTExternalMethod(method, flags, inputs, outputs) {0, kIOExternalMethodACIDPadding, method, flags, inputs, outputs}
#else
#error "Invalid architecture"
#endif

IOExternalMethodACID VoodooInputWellspringUserClient::sMethods[VoodooInputMT1UserClientMethodsNumMethods] = {
    // VoodooInputMT1UserClientMethodsSetSendsFrames
    MTExternalMethod((IOMethodACID) &VoodooInputWellspringUserClient::sSetSendFrames, kIOUCScalarIScalarO, 1, 0),
    // VoodooInputMT1UserClientMethodsGetReport
    MTExternalMethod((IOMethodACID) &VoodooInputWellspringUserClient::sGetReport, kIOUCStructIStructO, sizeof(MTDeviceReportStruct), sizeof(MTDeviceReportStruct)),
    // VoodooInputMT1UserClientMethodsSetReport
    MTExternalMethod((IOMethodACID) &VoodooInputWellspringUserClient::sNoop, kIOUCStructIStructO, sizeof(MTDeviceReportStruct), sizeof(MTDeviceReportStruct)),
    // VoodooInputMT1UserClientMethodsSetSendLogs
    MTExternalMethod((IOMethodACID) &VoodooInputWellspringUserClient::sNoop, kIOUCScalarIScalarO, 1, 0),
    // VoodooInputMT1UserClientMethodsIssueDriverRequest
    MTExternalMethod((IOMethodACID) &VoodooInputWellspringUserClient::sNoop, kIOUCStructIStructO, 0x204, 0x204),
    MTExternalMethod((IOMethodACID) &VoodooInputWellspringUserClient::sNoop, kIOUCScalarIScalarO, 3, 0),    // Relative Mouse Movement
    MTExternalMethod((IOMethodACID) &VoodooInputWellspringUserClient::sNoop, kIOUCScalarIScalarO, 3, 0),    // Scroll Wheel
    MTExternalMethod((IOMethodACID) &VoodooInputWellspringUserClient::sNoop, kIOUCScalarIScalarO, 2, 0),    // Keyboard
    MTExternalMethod((IOMethodACID) &VoodooInputWellspringUserClient::sNoop, kIOUCScalarIScalarO, 1, 0),    // Map Clicks
    // VoodooInputMT1UserClientMethodsRecacheProperties
    MTExternalMethod((IOMethodACID) &VoodooInputWellspringUserClient::sNoop, kIOUCScalarIScalarO, 0, 0),
    MTExternalMethod((IOMethodACID) &VoodooInputWellspringUserClient::sNoop, kIOUCScalarIScalarO, 3, 0),    // Momentum Scroll
};

bool VoodooInputWellspringUserClient::start(IOService *provider) {
    if (!super::start(provider)) return false;
    simulator = OSDynamicCast(VoodooInputWellspringSimulator, provider);
    
    if (simulator == nullptr) {
        IOLog("%s Invalid provider!\n", getName());
        return false;
    }
    
    dataQueue = IOSharedDataQueue::withCapacity(0x10004);
    // I haven't seen the log queue used yet.
    // Leaving this here with a small size in case userspace decides to ask for it at some point.
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
    IOLog("%s External Method %u\n", getName(), index);
    if (index >= VoodooInputMT1UserClientMethodsNumMethods) {
        return nullptr;
    }
    
    *targetP = this;
    return reinterpret_cast<IOExternalMethod *>(&sMethods[index]);
}

IOReturn VoodooInputWellspringUserClient::sSetSendFrames(VoodooInputWellspringUserClient *that, bool enableReports) {
    bool success = true;
    IOLog("%s Set Send Frames: %d\n", that->getName(), enableReports);
    
    if (enableReports) {
        success = that->simulator->registerUserClient(that);
    } else {
        that->simulator->unregisterUserClient(that);
    }
    
    return success ? kIOReturnSuccess : kIOReturnError;
}

// I'm not really sure why they use two different structs here???
IOReturn VoodooInputWellspringUserClient::sGetReport(VoodooInputWellspringUserClient *that, MTDeviceReportStruct *input, MTDeviceReportStruct *output) {
    if (input == nullptr || output == nullptr) {
        return kIOReturnBadArgument;
    }
    
    IOLog("%s Get Report: %u\n", that->getName(), input->reportId);
    
    IOReturn ret = that->simulator->getReport(input);
    if (ret == kIOReturnSuccess) {
        memmove(output->data, input->data, input->dataSize);
        output->dataSize = input->dataSize;
    }
    
    return ret;
}

void VoodooInputWellspringUserClient::enqueueData(void *data, size_t size) {
    if (dataQueue == nullptr) return;
    dataQueue->enqueue(data, (UInt32) size);
}

IOReturn VoodooInputWellspringUserClient::sNoop(VoodooInputWellspringUserClient *that, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    IOLog("%s Noop was called!\n", that->getName());
    return kIOReturnSuccess; // noop
}
