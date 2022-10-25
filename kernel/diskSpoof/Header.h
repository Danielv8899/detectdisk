#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include <windef.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <ata.h>
#include <scsi.h>
#include <ntddndis.h>
#include <mountmgr.h>
#include <mountdev.h>
#include <classpnp.h>
#include <ntimage.h>

NTKERNELAPI NTSTATUS ObReferenceObjectByName(IN PUNICODE_STRING ObjectName, IN ULONG Attributes, IN PACCESS_STATE PassedAccessState, IN ACCESS_MASK DesiredAccess, IN POBJECT_TYPE ObjectType, IN KPROCESSOR_MODE AccessMode, IN OUT PVOID ParseContext, OUT PVOID Object);
extern POBJECT_TYPE* IoDriverObjectType;

typedef struct _SWAP {
	UNICODE_STRING Name;
	PVOID* Swap;
	PVOID Original;
} SWAP, * PSWAP;

static struct {
	SWAP Buffer[0xFF];
	ULONG Length;
} SWAPS = { 0 };

#define AppendSwap(name, swap, hook, original) { \
	UNICODE_STRING _n = name; \
	PSWAP _s = &SWAPS.Buffer[SWAPS.Length++]; \
	*(PVOID *)&original = _s->Original = InterlockedExchangePointer((PVOID *)(_s->Swap = (PVOID *)swap), (PVOID)hook); \
	_s->Name = _n; \
}

typedef struct _IOC_REQUEST {
	PVOID Buffer;
	ULONG BufferLength;
	PVOID OldContext;
	PIO_COMPLETION_ROUTINE OldRoutine;
} IOC_REQUEST, * PIOC_REQUEST;