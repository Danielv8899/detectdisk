
#include "disk.h"


#define QWORD ULONGLONG
#define SMART_PARAMS_SIZE sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1

VOID SwapEndianess(PCHAR dest, PCHAR src) {
	for (size_t i = 0, l = strlen(src); i < l; i += 2) {
		dest[i] = src[i + 1];
		dest[i + 1] = src[i];
	}
}

VOID dontSwapEndianness(PCHAR dest, PCHAR src) {
	for (size_t i = 0, l = strlen(src); i < l; i++) {
		dest[i] = src[i];
	}
}

VOID ChangeIoc(PIO_STACK_LOCATION ioc, PIRP irp, PIO_COMPLETION_ROUTINE routine) {
	PIOC_REQUEST request = (PIOC_REQUEST)ExAllocatePool(NonPagedPool, sizeof(IOC_REQUEST));
	if (!request) {
		return;
	}

	request->Buffer = irp->AssociatedIrp.SystemBuffer;
	request->BufferLength = ioc->Parameters.DeviceIoControl.OutputBufferLength;
	request->OldContext = ioc->Context;
	request->OldRoutine = ioc->CompletionRoutine;

	ioc->Control = 0;
	ioc->Control |= SL_INVOKE_ON_SUCCESS;
	ioc->Context = request;
	ioc->CompletionRoutine = routine;
}

NTSTATUS StorageQueryIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (!context) {
		return STATUS_BAD_DATA;
	}
	const auto request = (PIOC_REQUEST)context;
	const auto buffer_length = request->BufferLength;
	const auto buffer = (PSTORAGE_DEVICE_DESCRIPTOR)request->Buffer;
	const auto old_context = request->OldContext;
	const auto old_routine = request->OldRoutine;

		ExFreePool(context);
		do {

			if (buffer_length < FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties))
				break;

			if (buffer->SerialNumberOffset == 0)
			{
				break;
			}

			if (buffer_length < FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties) + buffer->RawPropertiesLength
				|| buffer->SerialNumberOffset < FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties)
				|| buffer->SerialNumberOffset >= buffer_length){
				
				break;

				} 
			else{

				const auto serial = (PCHAR)buffer + buffer->SerialNumberOffset;
				const auto model = (PCHAR)buffer + buffer->ProductIdOffset;

				strcpy(serial, NAME);
				strcpy(model, NAME);

			}
		} while (false);
		//if (request->OldRoutine && irp->StackCount > 1) {
		//	return request->OldRoutine(device, irp, request->OldContext);
		//}
	
	if (irp->StackCount > 1ul && old_routine)
		return old_routine(device, irp, old_context);

	return STATUS_SUCCESS;
}

NTSTATUS SmartRcvIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context) {

	UNREFERENCED_PARAMETER(device);

	if (!context) {
		return STATUS_BAD_DATA;
	}

		const auto request = (PIOC_REQUEST)context;
		const auto buffer_length = request->BufferLength;
		const auto buffer = (SENDCMDOUTPARAMS*)request->Buffer;
		ExFreePool(context);

		if (buffer_length < FIELD_OFFSET(SENDCMDOUTPARAMS, bBuffer)
			|| FIELD_OFFSET(SENDCMDOUTPARAMS, bBuffer) + buffer->cBufferSize > buffer_length
			|| buffer->cBufferSize < sizeof(IDINFO)
			) {

			return STATUS_BAD_DATA;

		}

		else {

			const auto info = (PIDINFO)buffer->bBuffer;
			const auto serial = info->sSerialNumber;
			const auto model = info->sModelNumber;

			SwapEndianess(serial, NAME);
			SwapEndianess(model, NAME);
		}

		//if (request->OldRoutine && irp->StackCount > 1) {
		//	return request->OldRoutine(device, irp, request->OldContext);
		//}
	
	return irp->IoStatus.Status;
}
