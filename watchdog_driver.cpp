#include <ntddk.h>

#define IOCTL_PROTECT_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// The IOCTL input buffer will contain the PID of the process to protect.
typedef struct _PROTECT_PROCESS_INPUT_BUFFER {
    ULONG ProcessId;
} PROTECT_PROCESS_INPUT_BUFFER, *PPROTECT_PROCESS_INPUT_BUFFER;

// The IOCTL output buffer will contain the status of the operation.
typedef struct _PROTECT_PROCESS_OUTPUT_BUFFER {
    NTSTATUS Status;
} PROTECT_PROCESS_OUTPUT_BUFFER, *PPROTECT_PROCESS_OUTPUT_BUFFER;

NTSTATUS ProtectProcess(IN PPROTECT_PROCESS_INPUT_BUFFER InputBuffer, IN ULONG InputBufferLength, OUT PPROTECT_PROCESS_OUTPUT_BUFFER OutputBuffer, IN ULONG OutputBufferLength)
{
    // Validate the input and output buffers.
    if (InputBuffer == NULL || InputBufferLength < sizeof(PROTECT_PROCESS_INPUT_BUFFER) || OutputBuffer == NULL || OutputBufferLength < sizeof(PROTECT_PROCESS_OUTPUT_BUFFER)) {
        return STATUS_INVALID_PARAMETER;
    }

    // Open a handle to the target process.
    HANDLE ProcessHandle = NULL;
    NTSTATUS status = ZwOpenProcess(&ProcessHandle, PROCESS_SET_INFORMATION, NULL, &InputBuffer->ProcessId);
    if (!NT_SUCCESS(status)) {
        OutputBuffer->Status = status;
        return status;
    }

    // Set the protection level of the process to PROTECTED_PROCESS.
    ULONG ProtectLevel = 4; // PROTECTED_PROCESS
    status = ZwSetInformationProcess(ProcessHandle, ProcessProtectionInformation, &ProtectLevel, sizeof(ProtectLevel));
    if (!NT_SUCCESS(status)) {
        OutputBuffer->Status = status;
        return status;
    }

    // Close the handle to the target process.
    ZwClose(ProcessHandle);

    // Return success.
    OutputBuffer->Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS DispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

    NTSTATUS status = STATUS_SUCCESS;
    ULONG info = 0;

    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_PROTECT_PROCESS:
        status = ProtectProcess((PPROTECT_PROCESS_INPUT_BUFFER)Irp->AssociatedIrp.SystemBuffer, IrpStack->Parameters.DeviceIoControl.InputBufferLength, (PPROTECT_PROCESS_OUTPUT_BUFFER)Irp->AssociatedIrp.SystemBuffer, IrpStack->Parameters.DeviceIoControl.OutputBufferLength);
        info = sizeof(PROTECT_PROCESS_OUTPUT_BUFFER);
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    UNICODE_STRING deviceName, symLinkName;

    // Create a device object for the driver.
    RtlInitUnicodeString(&deviceName, L"\\Device\\ProtectProcessDriver");
    status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DriverObject->DeviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create a symbolic link for the device object.
    RtlInitUnicodeString(&symLinkName, L"\\DosDevices\\ProtectProcessDriver");
    status = IoCreateSymbolicLink(&symLinkName, &deviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(DriverObject->DeviceObject);
        return status;
    }

    // Set the dispatch routines for the driver.
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;

    return STATUS_SUCCESS;
}

