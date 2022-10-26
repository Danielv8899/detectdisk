#include "disk.h"

#define QWORD ULONGLONG
#define SMART_PARAMS_SIZE sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1

VOID ChangeIoc(PIO_STACK_LOCATION ioc, PIRP irp, PIO_COMPLETION_ROUTINE routine) {
	PIOC_REQUEST request = (PIOC_REQUEST)ExAllocatePool2(NonPagedPool, sizeof(IOC_REQUEST), 0x6e556353);
	if (!request) {
		return;
	}

	request->Buffer = irp->AssociatedIrp.SystemBuffer;
	request->BufferLength = ioc->Parameters.DeviceIoControl.OutputBufferLength;
	request->OldContext = ioc->Context;
	request->OldRoutine = ioc->CompletionRoutine;

	ioc->Control = SL_INVOKE_ON_SUCCESS;
	ioc->Context = request;
	ioc->CompletionRoutine = routine;
}

NTSTATUS StorageQueryIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (context) {
		IOC_REQUEST request = *(PIOC_REQUEST)context;
		ExFreePool(context);

		if (request.BufferLength >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
			PSTORAGE_DEVICE_DESCRIPTOR desc = (PSTORAGE_DEVICE_DESCRIPTOR)request.Buffer;
			ULONG offset = desc->VendorIdOffset;
			ULONG offset2 = desc->ProductIdOffset;
			ULONG offset3 = desc->ProductRevisionOffset;
			ULONG offset4 = desc->SerialNumberOffset;
			if (offset && offset < request.BufferLength) {
				strcpy((PCHAR)desc + offset, NAME);
				strcpy((PCHAR)desc + offset2, NAME);
				strcpy((PCHAR)desc + offset3, NAME);
				strcpy((PCHAR)desc + offset4, NAME);
			}
		}
		if (request.OldRoutine && irp->StackCount > 1) {
			return request.OldRoutine(device, irp, request.OldContext);
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS SmartRcvIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (context) {
		IOC_REQUEST request = *(PIOC_REQUEST)context;
		ExFreePool(context);

		if (request.BufferLength >= sizeof(PSENDCMDINPARAMS)) {
			PSENDCMDINPARAMS desc = (PSENDCMDINPARAMS)request.Buffer;
			QWORD offset = (QWORD)desc->bBuffer - (QWORD)desc;
			if (offset && offset < request.BufferLength) {
				strcpy((PCHAR)desc + offset, NAME);
			}
		}
		if (request.OldRoutine && irp->StackCount > 1) {
			return request.OldRoutine(device, irp, request.OldContext);
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS DiskControl(PDEVICE_OBJECT device, PIRP irp) {
	PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);
	PVOID buffer = irp->AssociatedIrp.SystemBuffer;
	switch (ioc->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_STORAGE_QUERY_PROPERTY:
		if (StorageDeviceProperty == ((PSTORAGE_PROPERTY_QUERY)buffer)->PropertyId) {
			ChangeIoc(ioc, irp, StorageQueryIoc);
		}
		break;
	
	case SMART_RCV_DRIVE_DATA:
	{
		PSENDCMDINPARAMS pCmd = (PSENDCMDINPARAMS)buffer;
		if (pCmd->irDriveRegs.bCommandReg == ID_CMD) {
			ChangeIoc(ioc, irp, SmartRcvIoc);
		}
	}
	}
	return DiskControlOriginal(device, irp);
}