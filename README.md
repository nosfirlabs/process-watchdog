# process-watchdog
C++ driver to protect a process with a given PID 

This driver exports a single IOCTL (Input/Output Control) with the code IOCTL_PROTECT_PROCESS, which takes an input buffer containing the PID of the process to protect, and an output buffer containing the status of the operation.

The ProtectProcess function opens a handle to the target process, sets the protection level of the process to PROTECTED_PROCESS, and then closes the handle. The PROTECTED_PROCESS level is a protection level introduced in Windows 10 that allows a process to be protected from termination and injection by low integrity processes.

The DispatchIoctl function is the dispatch routine for the IRP_MJ_DEVICE_CONTROL IRP (I/O Request Packet) major function. It handles the IOCTL_PROTECT_PROCESS IOCTL by calling the ProtectProcess function and setting the status and information fields of the IRP.

The DriverEntry function is the entry point for the driver. It creates a device object and a symbolic link for the device, and sets the dispatch routines for the driver.

To use this driver, you would need to build it as a kernel-mode driver and install it on your system. You can then open a handle to the device object and issue the IOCTL_PROTECT_PROCESS IOCTL to protect a process
