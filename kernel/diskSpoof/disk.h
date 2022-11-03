#pragma once
#include "Header.h"

static CHAR* NAME = "TESTDRV";
#define DEBUGPRINT "[-] diskSpoof:"

VOID ChangeIoc(PIO_STACK_LOCATION ioc, PIRP irp, PIO_COMPLETION_ROUTINE routine);
NTSTATUS StorageQueryIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context);
NTSTATUS DiskControl(PDEVICE_OBJECT device, PIRP irp);
NTSTATUS SmartRcvIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context);