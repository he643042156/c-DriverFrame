/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_Test,
    0x93276cdf,0x91f0,0x4d80,0x95,0x80,0x59,0x9c,0x28,0x22,0xde,0xeb);
// {93276cdf-91f0-4d80-9580-599c2822deeb}
#define IOCTL_READ CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

#define MYWDF_KDEVICE L"\\Device\\MyWDF_Device"//设备名称，其他内核模式下的驱动可以使用  
#define MYWDF_LINKNAME L"\\DosDevices\\MyWDF_LINK"//符号连接，这样用户模式下的程序可以使用这个驱动设备。

#define COUNT 1024

#define SIZE (1024)

#pragma warning(disable:4100)