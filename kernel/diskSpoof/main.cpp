#pragma once
#include "disk.h"
#include "Header.h"

PDRIVER_DISPATCH g_original_device_control;

VOID DriverUnload(PDRIVER_OBJECT driver) {
	UNREFERENCED_PARAMETER(driver);

}

NTSTATUS DiskControl(PDEVICE_OBJECT device, PIRP irp) {
	PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);
	PVOID buffer = irp->AssociatedIrp.SystemBuffer;
	switch (ioc->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_STORAGE_QUERY_PROPERTY:
	{
		if (((PSTORAGE_PROPERTY_QUERY)buffer)->PropertyId == StorageDeviceProperty)
			ChangeIoc(ioc, irp, StorageQueryIoc);
	}
	break;

	case SMART_RCV_DRIVE_DATA:
	{
		ChangeIoc(ioc, irp, SmartRcvIoc);
	}
	break;
	default:
		break;
	}
	return g_original_device_control(device, irp);
}

NTSTATUS doHook() {
	UNICODE_STRING diskstr = RTL_CONSTANT_STRING(L"\\Driver\\Disk");
	PDRIVER_OBJECT diskobj = 0;
	auto status = ObReferenceObjectByName(&diskstr, OBJ_CASE_INSENSITIVE, 0, 0, *IoDriverObjectType, KernelMode, 0, (PVOID*)&diskobj);

	if (!NT_SUCCESS(status) || !diskobj) {
		DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"%s failed getting disk object\n", DEBUGPRINT);
		return STATUS_ACCESS_DENIED;
	}

	auto& device_control = diskobj->MajorFunction[IRP_MJ_DEVICE_CONTROL];
	g_original_device_control = device_control;
	device_control = &DiskControl;

	ObDereferenceObject(diskobj);
	return STATUS_SUCCESS;
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	driver->DriverUnload = DriverUnload;

	return doHook();
}