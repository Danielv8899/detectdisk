#pragma once
#include "Header.h"

static CHAR* NAME = "TESTDRV";
PDRIVER_DISPATCH DiskControlOriginal;

VOID ChangeIoc(PIO_STACK_LOCATION ioc, PIRP irp, PIO_COMPLETION_ROUTINE routine);
NTSTATUS StorageQueryIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context);
NTSTATUS DiskControl(PDEVICE_OBJECT device, PIRP irp);