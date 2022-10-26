
#include <iostream>
#include <string>
#include <Windows.h>
#include <winioctl.h>
#include <list>

#define  DFP_GET_VERSION          0x00074080
#define  IDE_ATAPI_IDENTIFY  0xA1  
#define  IDE_ATA_IDENTIFY    0xEC 
#define  DFP_RECEIVE_DRIVE_DATA   0x0007c088

typedef struct _GETVERSIONOUTPARAMS {
    BYTE bVersion;     
    BYTE bRevision;    
    BYTE bReserved;     
    BYTE bIDEDeviceMap; 
    DWORD fCapabilities; 
    DWORD dwReserved[4]; 
} GETVERSIONOUTPARAMS, * PGETVERSIONOUTPARAMS, * LPGETVERSIONOUTPARAMS;

bool drv_convert_to_string(DWORD diskdata[256], DWORD diskdata_size, unsigned int firstIndex, unsigned int lastIndex, std::string& buffer) {
    unsigned int index = 0;

    if (firstIndex > lastIndex || firstIndex >= diskdata_size || lastIndex >= diskdata_size)
        return false;

    for (index = firstIndex; index <= lastIndex; index++) {
        buffer += static_cast<char>((diskdata[index] / 256));
        buffer += static_cast<char>((diskdata[index] % 256));
    }

    return true;
}


int main()
{
    STORAGE_PROPERTY_QUERY pQuery;
    STORAGE_DESCRIPTOR_HEADER pHdr = { 0 };
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    BOOL bResult = FALSE;
    DWORD dwBytesReturned = 0;
    BYTE* pOutBuffer = NULL;
    std::list<std::string> drvModelName;
    char a;

    std::wstring device = L"\\\\.\\PhysicalDrive0";
    ZeroMemory(&pQuery, sizeof(STORAGE_PROPERTY_QUERY));
    pQuery.PropertyId = StorageDeviceProperty;
    pQuery.QueryType = PropertyStandardQuery;

    hDevice = CreateFileW(device.c_str() , GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::cout << GetLastError() << std::endl;
        std::cin >> a;
        return -1;
    }

    bResult = DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &pQuery, sizeof(STORAGE_PROPERTY_QUERY), &pHdr, sizeof(STORAGE_DESCRIPTOR_HEADER), &dwBytesReturned, NULL);
        if (!bResult){
            std::cout << GetLastError() << std::endl;;
            std::cin >> a;
            return -1;
        }

    const DWORD dwOutBufferSize = pHdr.Size;
    pOutBuffer = new BYTE[dwOutBufferSize];
    ZeroMemory(pOutBuffer, dwOutBufferSize);

    if(!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &pQuery, sizeof(STORAGE_PROPERTY_QUERY), pOutBuffer, dwOutBufferSize, &dwBytesReturned, NULL)){
            std::cout << GetLastError() << std::endl;;
            std::cin >> a;
            return -1;
        }

    STORAGE_DEVICE_DESCRIPTOR* pDevDesc = (STORAGE_DEVICE_DESCRIPTOR*)pOutBuffer;

    const DWORD dwVendorIdOffset = pDevDesc->VendorIdOffset;
    const DWORD dwProdIdOffset = pDevDesc->ProductIdOffset;
    UCHAR* strVendorId = 0, * strProdId = 0;

    if (dwProdIdOffset > 0) {
        strProdId = pOutBuffer + dwProdIdOffset;
        drvModelName.push_back(reinterpret_cast<char*>(strProdId));
    }

    if (dwVendorIdOffset > 0) {
        strVendorId = pOutBuffer + dwVendorIdOffset;
        drvModelName.push_back(reinterpret_cast<char*>(strVendorId));
    }
    auto st = drvModelName.begin();
    std::cout << "prod id: " << *st << std::endl;
    std::cin >> a;

    GETVERSIONOUTPARAMS VersionParams;
    DWORD cbBytesReturned = 0;
    BYTE IdOutCmd[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];

    memset((void*)&VersionParams, 0, sizeof(VersionParams));

    if (!DeviceIoControl(hDevice, DFP_GET_VERSION, NULL, 0, &VersionParams, sizeof(VersionParams), &cbBytesReturned, NULL)) {
        CloseHandle(hDevice);
        std::cout << "failed DFP_GET_VERSION" << std::endl;
        std::cin >> a;
        return -1;
    }

    BYTE bIDCmd = 0;
    SENDCMDINPARAMS scip;

    bIDCmd = (VersionParams.bIDEDeviceMap >> 0 & 0x10) ? IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;

    memset(&scip, 0, sizeof(scip));
    memset(IdOutCmd, 0, sizeof(IdOutCmd));

    PSENDCMDINPARAMS pSCIP = &scip;
    PSENDCMDOUTPARAMS pSCOP = reinterpret_cast<PSENDCMDOUTPARAMS>(&IdOutCmd);
    BYTE bbIDCmd = static_cast<BYTE>(bIDCmd);
    PDWORD lpcbBytesReturned = 0;


    pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;
    pSCIP->irDriveRegs.bFeaturesReg = 0;
    pSCIP->irDriveRegs.bSectorCountReg = 1;
    pSCIP->irDriveRegs.bCylLowReg = 0;
    pSCIP->irDriveRegs.bCylHighReg = 0;

    pSCIP->irDriveRegs.bDriveHeadReg = 0xA0 | ((0 & 1) << 4);

    pSCIP->irDriveRegs.bCommandReg = bbIDCmd;
    pSCIP->bDriveNumber = 0;
    pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;

    if (!(DeviceIoControl(
        hDevice,
        DFP_RECEIVE_DRIVE_DATA,
        static_cast<LPVOID>(pSCIP),
        sizeof(SENDCMDINPARAMS) - 1,
        static_cast<LPVOID>(pSCOP),
        sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
        lpcbBytesReturned, NULL))) {

        CloseHandle(hDevice);
        std::cout << "failed DFP_RECEIVE_DRIVE_DATA" << std::endl;
        std::cin >> a;
        return -1;
    }

    DWORD diskdata[256];
    USHORT* pIdSector = reinterpret_cast<USHORT*>((reinterpret_cast<PSENDCMDOUTPARAMS>(IdOutCmd))->bBuffer);
    std::string drive_model;

    for (int i = 0; i < 256; i++) {
        diskdata[i] = pIdSector[i];
    }

    drv_convert_to_string(diskdata, _countof(diskdata), 27, 46, drive_model);
    std::cout << "drive_model: " << drive_model.c_str() << std::endl;
    std::cin >> a;

    CloseHandle(hDevice);
    if (pOutBuffer) {
        delete[] pOutBuffer;
        pOutBuffer = NULL;
    }
    return 0;
}
