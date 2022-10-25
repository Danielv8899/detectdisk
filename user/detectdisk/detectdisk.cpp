
#include <iostream>
#include <string>
#include <Windows.h>
#include <winioctl.h>
#include <list>

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
    DWORD lastErr = GetLastError();

    std::wstring device = L"\\\\.\\PhysicalDrive0";
    ZeroMemory(&pQuery, sizeof(STORAGE_PROPERTY_QUERY));
    pQuery.PropertyId = StorageDeviceProperty;
    pQuery.QueryType = PropertyStandardQuery;

    hDevice = CreateFileW(device.c_str() , 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::cout << lastErr;
        std::cin >> a;
        return -1;
    }

    bResult = DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &pQuery, sizeof(STORAGE_PROPERTY_QUERY), &pHdr, sizeof(STORAGE_DESCRIPTOR_HEADER), &dwBytesReturned, NULL);
        if (!bResult){
            std::cout << lastErr;
            std::cin >> a;
            return -1;
        }

    const DWORD dwOutBufferSize = pHdr.Size;
    pOutBuffer = new BYTE[dwOutBufferSize];
    ZeroMemory(pOutBuffer, dwOutBufferSize);

    if(!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &pQuery, sizeof(STORAGE_PROPERTY_QUERY), pOutBuffer, dwOutBufferSize, &dwBytesReturned, NULL)){
            std::cout << lastErr;
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
    CloseHandle(hDevice);
    if (pOutBuffer) {
        delete[] pOutBuffer;
        pOutBuffer = NULL;
    }
    return 0;
}
