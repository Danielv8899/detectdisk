#pragma once
#include "disk.h"
#include "Header.h"

VOID DriverUnload(PDRIVER_OBJECT driver) {
	UNREFERENCED_PARAMETER(driver);

	for (DWORD i = 0; i < SWAPS.Length; ++i) {
		PSWAP s = (PSWAP)&SWAPS.Buffer[i];
		if (s->Swap && s->Original) {
			InterlockedExchangePointer(s->Swap, s->Original);
		}
	}
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	driver->DriverUnload = DriverUnload;

	UNICODE_STRING diskstr = RTL_CONSTANT_STRING(L"\\Driver\\Disk");
	PDRIVER_OBJECT diskobj = 0;
	NTSTATUS status = ObReferenceObjectByName(&diskstr, OBJ_CASE_INSENSITIVE, 0, 0, *IoDriverObjectType, KernelMode, 0, &diskobj);

	if (!NT_SUCCESS(status)) {
		return STATUS_ACCESS_DENIED;
	}

	AppendSwap(diskstr, &diskobj->MajorFunction[IRP_MJ_DEVICE_CONTROL], DiskControl, DiskControlOriginal);

	return STATUS_SUCCESS;
}