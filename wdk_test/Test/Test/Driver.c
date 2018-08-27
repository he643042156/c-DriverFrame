/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, TestEvtDeviceAdd)
#pragma alloc_text (PAGE, TestEvtDriverContextCleanup)
#endif


static VOID EvtDriverUnload(WDFDRIVER Driver)
{
	KdPrint(("unload driver\n"));
	KdPrint(("Doesn't need to clean up the devices, since we only have control device here\n"));
}/* EvtDriverUnload */

VOID EvtDeviceFileCreate(__in WDFDEVICE Device, __in WDFREQUEST Request, __in WDFFILEOBJECT FileObject)
{
	KdPrint(("EvtDeviceFileCreate"));

	WdfRequestComplete(Request, STATUS_SUCCESS);
}

VOID EvtFileClose(__in  WDFFILEOBJECT FileObject)
{
	KdPrint(("EvtFileClose"));
}

static VOID EvtIoPDPControlDevice(WDFQUEUE Queue, WDFREQUEST Request)
{

	WDF_REQUEST_PARAMETERS Params;
//	WDFREQUEST req;
	NTSTATUS status;
	WDFDEVICE  hDevice;
	DEVICE_CONTEXT* dev_ctx = NULL;

	//buffer  
	char* in_buf = NULL;
	char* out_buf = NULL;

	size_t out_size = 0;
	size_t in_size = 0;

	WDF_REQUEST_PARAMETERS_INIT(&Params);
	WdfRequestGetParameters(Request, &Params);

	KdPrint(("EvtIoPDPControlDevice, type: %x\n", Params.Type));

	status = WdfRequestRetrieveInputBuffer(Request, 0, &in_buf, &in_size);
	KdPrint(("Get input buffer, ret: %x, buffer: %x size: %d", status, in_buf, in_size));

	status = WdfRequestRetrieveOutputBuffer(Request, 0, &out_buf, &out_size);
	KdPrint(("Get output buffer, ret: %x, buffer: %x size: %d", status, out_buf, out_size));

	hDevice = WdfIoQueueGetDevice(Queue);
	dev_ctx = WdfObjectGetTypedContext(hDevice, DEVICE_CONTEXT);
	KdPrint(("Get the context of current device, %x", dev_ctx));

	switch (Params.Type)
	{
	case WdfRequestTypeRead:
	{
		if (out_buf != NULL)
		{
			out_buf[0] = '1';
			KdPrint(("write the data in context buffer into READ request\n"));
			//将设备上下文里面的数据写入Read请求的缓冲里面  
			RtlCopyMemory(out_buf, dev_ctx->_Buffer, SIZE);
			WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, SIZE);
		}
	}
	break;
	case WdfRequestTypeWrite:
	{
		if (in_buf != NULL)
		{
			RtlCopyMemory(dev_ctx->_Buffer, in_buf, SIZE);
			WdfRequestComplete(Request, STATUS_SUCCESS);
		}
	}
	break;
	default:
		WdfRequestComplete(Request, STATUS_SUCCESS);
		break;
	}

}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES object_attribs;
	PWDFDEVICE_INIT device_init = NULL;
	WDFDRIVER drv = NULL;//wdf framework 驱动对象
	UNICODE_STRING ustring;
	WDFDEVICE control_device;
	DEVICE_CONTEXT* dev_ctx = NULL;
	WDF_FILEOBJECT_CONFIG f_cfg;
	WDF_IO_QUEUE_CONFIG qcfg;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING( DriverObject, RegistryPath );

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");


	WDF_OBJECT_ATTRIBUTES_INIT(&object_attribs);
	object_attribs.EvtCleanupCallback = TestEvtDriverContextCleanup;

	WDF_DRIVER_CONFIG_INIT(&config,
		//TestEvtDeviceAdd
		NULL
	);

	config.DriverInitFlags = WdfDriverInitNonPnpDriver;  //指定非pnp驱动  
	config.DriverPoolTag = (ULONG)'PEPU';
	config.EvtDriverUnload = EvtDriverUnload;  //指定卸载函数  

	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		WDF_NO_OBJECT_ATTRIBUTES,
		&config,
		&drv
	);
    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

	KdPrint(("aaaaaaaCreate wdf driver object successfully\n"));

	//先要分配一块内存WDFDEVICE_INIT,这块内存在创建设备的时候会用到。  
	device_init = WdfControlDeviceInitAllocate(drv, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R);
	if (device_init == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto DriverEntry_Complete;
	}

	RtlInitUnicodeString(&ustring, MYWDF_KDEVICE);
	status = WdfDeviceInitAssignName(device_init, &ustring);
	if (!NT_SUCCESS(status))
	{
		goto DriverEntry_Complete;
	}
	KdPrint(("Device name Unicode string: %wZ (this name can only be used by other kernel mode code, like other drivers)\n", &ustring));

	WDF_FILEOBJECT_CONFIG_INIT(&f_cfg, EvtDeviceFileCreate, EvtFileClose, NULL);
	WdfDeviceInitSetFileObjectConfig(device_init, &f_cfg, WDF_NO_OBJECT_ATTRIBUTES);

	//自定义结构
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&object_attribs, DEVICE_CONTEXT);


	//根据前面创建的device_init来创建一个设备.
	status = WdfDeviceCreate(&device_init, &object_attribs, &control_device);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("create device failed\n"));
		goto DriverEntry_Complete;
	}
	dev_ctx = WdfObjectGetTypedContext(control_device, DEVICE_CONTEXT);
	RtlZeroMemory(dev_ctx, sizeof(DEVICE_CONTEXT));

	//创建IO queue  
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&qcfg, WdfIoQueueDispatchParallel);
	qcfg.PowerManaged = WdfFalse;  //非pnp驱动，无需电源管理。  
	qcfg.EvtIoDefault = EvtIoPDPControlDevice;
	//qcfg.EvtIoDeviceControl = TestEvtIoDeviceControl;

	//给设备control_device创建IO QUEUE  
	status = WdfIoQueueCreate(control_device, &qcfg, WDF_NO_OBJECT_ATTRIBUTES, &dev_ctx->_DefaultQueue);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Create IoQueue failed, %x\n", status));
		goto DriverEntry_Complete;
	}

	//创建符号连接
	RtlInitUnicodeString(&ustring, MYWDF_LINKNAME);
	status = WdfDeviceCreateSymbolicLink(control_device, &ustring);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to create Link\n"));
		goto DriverEntry_Complete;
	}
	KdPrint(("Create symbolic link successfully, %wZ (user mode code should use this name, like in CreateFile())\n", &ustring));

	WdfControlFinishInitializing(control_device);//创建设备完成。

DriverEntry_Complete:

    return status;
}

NTSTATUS
TestEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    status = TestCreateDevice(DeviceInit);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

VOID
TestEvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
    )
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE ();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Stop WPP Tracing
    //
    WPP_CLEANUP( WdfDriverWdmGetDriverObject( (WDFDRIVER) DriverObject) );

}
