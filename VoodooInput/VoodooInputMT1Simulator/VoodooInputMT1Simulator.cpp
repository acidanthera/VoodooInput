//
//  AppleUSBMultitouchDriver.cpp
//  VoodooInput
//
//  Created by Avery Black on 12/31/22.
//  Copyright © 2022 Kishor Prins. All rights reserved.
//

#include "VoodooInputMT1Simulator.hpp"

#define super IOService
OSDefineMetaClassAndStructors(VoodooInputMT1Simulator, IOService);

bool VoodooInputMT1Simulator::start(IOService *provider) {
    if (!super::start(provider)) return false;
    
    engine = OSDynamicCast(VoodooInput, provider);
    if (!engine) {
        return false;
    }
    
    workloop = getWorkLoop();
    if (workloop == nullptr) {
        IOLog("MT1Sim: Failed to create work loop!\n");
        return false;
    }
    
    workloop->retain();
    
    cmdGate = OSTypeAlloc(IOCommandGate);
    if (cmdGate == nullptr) {
        IOLog("MT1Sim: Failed to create cmd gate\n");
        return false;
    }
    
    workloop->addEventSource(cmdGate);
    
    userClients = OSSet::withCapacity(1);
    if (userClients == nullptr) {
        IOLog("MT1Sim: Failed to create user clients array\n");
        return false;
    }
    
    // Set name so that WindowServer attaches to us
    setName("AppleUSBMultitouchDriver");
    setProperty(kIOUserClientClassKey, "VoodooInputMT1UserClient");
    registerService();
    
    return true;
}

void VoodooInputMT1Simulator::stop(IOService *provider) {
    if (workloop) {
        workloop->removeEventSource(cmdGate);
    }
    
    OSSafeReleaseNULL(workloop);
    OSSafeReleaseNULL(cmdGate);
    if (userClients != nullptr) {
        userClients->flushCollection();
        OSSafeReleaseNULL(userClients);
    }
    
    super::stop(provider);
}

bool VoodooInputMT1Simulator::registerUserClient(IOService *client) {
    IOLog("MT1Sim: Adding user client\n");
    return userClients->setObject(client);
}

void VoodooInputMT1Simulator::unregisterUserClient(IOService *client) {
    IOLog("MT1Sim: Removing user client\n");
    userClients->removeObject(client);
}

// Reports come from a MacbookPro9,2
#define SensorParamsLength 6
static const UInt8 MT1SensorParams[SensorParamsLength] = {0x00, 0x00, 0x03, 0x00, 0xD6, 0x01};

#define SensorDescLength 8
static const UInt8 MT1SensorDesc[SensorDescLength] = {0x01, 0x01, 0x00, 0x0c, 0x01, 0x00, 0x14, 0x00};

#define SensorRowsLength 5
static const UInt8 MT1SensorRows[SensorRowsLength] = {
    0x01,       // Endianness (no clue what 1 means)
    0x0C,       // Rows
    0x14,       // Columns
    0x01, 0x09  // BCD Version
};

#define FamilyID 0x62
#define FamilyIDLength 1

IOReturn VoodooInputMT1Simulator::getReport(MT1DeviceReportStruct *toFill) {
    UInt32 width, height;
    
    switch (toFill->reportId) {
        case MT1ReportSensorParam:
            memcpy(toFill->data, MT1SensorParams, SensorParamsLength);
            toFill->dataSize = SensorParamsLength;
            break;
        case MT1ReportSensorDescriptor:
            memcpy(toFill->data, MT1SensorDesc, SensorDescLength);
            toFill->dataSize = SensorDescLength;
            break;
        case MT1ReportSensorSize:
            width = engine->getPhysicalMaxX();
            height = engine->getPhysicalMaxY();
            
            for (int i = 0; i < sizeof(UInt32); i++) {
                toFill->data[i] = width & 0xFF;
                width >>= 8;
            }
            
            for (int i = 4; i < sizeof(UInt32) + 4; i++) {
                toFill->data[i] = height & 0xFF;
                height >>= 8;
            }
            
            toFill->dataSize = sizeof(UInt32) * 2;
            break;
        case MT1ReportSensorRows:
            memcpy(toFill->data, MT1SensorRows, SensorRowsLength);
            toFill->dataSize = SensorRowsLength;
            break;
        case MT1ReportFamilyId:
            toFill->data[0] = FamilyID;
            toFill->dataSize = FamilyIDLength;
            break;
            
        default: return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}
