#include <ntddk.h>
#include <wdf.h>
#include "Common.h"
#include "HypervisorRoutines.h"
#include "GlobalVariables.h"
#include "Logging.h"
#include "Hooks.h"
#include "Trace.h"
#include "Driver.tmh"

/* Main Driver Entry in the case of driver load */
/* 驱动程序加载情况下的主驱动程序入口 */
NTSTATUS DriverEntry(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING  RegistryPath)
{
	NTSTATUS Ntstatus = STATUS_SUCCESS;
	UINT64 Index = 0;
	PDEVICE_OBJECT DeviceObject = NULL;
	UNICODE_STRING DriverName, DosDeviceName;
	int ProcessorCount;

	UNREFERENCED_PARAMETER(RegistryPath);
	UNREFERENCED_PARAMETER(DriverObject);

	// Initialize WPP Tracing
    // 初始化 WPP 跟踪
	WPP_INIT_TRACING(DriverObject, RegistryPath);

#if !UseDbgPrintInsteadOfUsermodeMessageTracking 
	if (!LogInitialize())
	{
		DbgPrint("[*] Log buffer is not initialized !\n");
		DbgBreakPoint();
	}
#endif

	// Opt-in to using non-executable pool memory on Windows 8 and later.
    // 在 Windows 8 及更高版本上选择使用非可执行池内存。
	// https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

	/////////////// we allocate virtual machine here because we want to use its state (vmx-root or vmx non-root) in logs \\\\\\\\\\\\\\\
	/////////////// 在这里分配虚拟机，因为我们希望在日志中使用其状态（vmx-root 或 vmx non-root）。 \\\\\\\\

	ProcessorCount = KeQueryActiveProcessorCount(0);

	// Allocate global variable to hold Guest(s) state
    // 分配全局变量以保存虚拟机的状态
	GuestState = ExAllocatePoolWithTag(NonPagedPool, sizeof(VIRTUAL_MACHINE_STATE) * ProcessorCount, POOLTAG);

	if (!GuestState)
	{
		// we use DbgPrint as the vmx-root or non-root is not initialized
        // 由于 vmx-root 或 non-root 未初始化，我们使用 DbgPrint。
		DbgPrint("Insufficient memory\n");
		DbgBreakPoint();
		return FALSE;
	}

	// Zero memory
    // 清零内存
	RtlZeroMemory(GuestState, sizeof(VIRTUAL_MACHINE_STATE) * ProcessorCount);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	LogInfo("Hypervisor From Scratch Loaded :)");

	RtlInitUnicodeString(&DriverName, L"\\Device\\MyHypervisorDevice");

	RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\MyHypervisorDevice");

	Ntstatus = IoCreateDevice(DriverObject, 0, &DriverName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);

	if (Ntstatus == STATUS_SUCCESS)
	{
		for (Index = 0; Index < IRP_MJ_MAXIMUM_FUNCTION; Index++)
			DriverObject->MajorFunction[Index] = DrvUnsupported;

		LogInfo("Setting device major functions");
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = DrvClose;
		DriverObject->MajorFunction[IRP_MJ_CREATE] = DrvCreate;
		DriverObject->MajorFunction[IRP_MJ_READ] = DrvRead;
		DriverObject->MajorFunction[IRP_MJ_WRITE] = DrvWrite;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DrvDispatchIoControl;

		DriverObject->DriverUnload = DrvUnload;
		IoCreateSymbolicLink(&DosDeviceName, &DriverName);
	}

	// Establish user-buffer access method.
    // 建立用户缓冲区访问方法。
	DeviceObject->Flags |= DO_BUFFERED_IO;

	ASSERT(NT_SUCCESS(Ntstatus));
	return Ntstatus;
}

/* Run in the case of driver unload to unregister the devices */
/* 在驱动程序卸载的情况下运行，以注销设备 */
VOID DrvUnload(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING DosDeviceName;


	RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\MyHypervisorDevice");
	IoDeleteSymbolicLink(&DosDeviceName);
	IoDeleteDevice(DriverObject->DeviceObject);

	DbgPrint("Hypervisor From Scratch's driver unloaded\n");

#if !UseDbgPrintInsteadOfUsermodeMessageTracking 
	// Uinitialize log buffer
    // 反初始化日志缓冲区
	DbgPrint("Uinitializing logs\n");
	LogUnInitialize();
#endif
	// Free GuestState 
	// 释放 GuestState 资源
	ExFreePoolWithTag(GuestState, POOLTAG);

	// Stop the tracing
    // 停止跟踪
	WPP_CLEANUP(DriverObject);

}


/* IRP_MJ_CREATE Function handler*/
/* IRP_MJ_CREATE 函数处理程序 */
NTSTATUS DrvCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	int ProcessorCount;

	// Allow to server IOCTL 
	// 允许服务 IOCTL 请求
	AllowIOCTLFromUsermode = TRUE;

	LogInfo("Hypervisor From Scratch Started...");


	/* We have to zero the GuestState again as we want to support multiple initialization by CreateFile */
    /* 由于我们要支持通过 CreateFile 进行多次初始化，我们必须再次将 GuestState 清零 */
	ProcessorCount = KeQueryActiveProcessorCount(0);
	// Zero memory
	RtlZeroMemory(GuestState, sizeof(VIRTUAL_MACHINE_STATE) * ProcessorCount);


	if (HvVmxInitialize())
	{
		LogInfo("Hypervisor From Scratch loaded successfully :)");
	}
	else
	{
		LogError("Hypervisor From Scratch was not loaded :(");
	}

	//////////// test //////////// 
	//HiddenHooksTest();
	//SyscallHookTest();
	////////////////////////////// 

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/* IRP_MJ_READ Function handler*/
/* IRP_MJ_READ 函数处理程序 */
NTSTATUS DrvRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	LogWarning("Not implemented yet :(");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/* IRP_MJ_WRITE Function handler*/
/* IRP_MJ_WRITE 函数处理程序 */
NTSTATUS DrvWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	LogWarning("Not implemented yet :(");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/* IRP_MJ_CLOSE Function handler*/
/* IRP_MJ_CLOSE 函数处理程序 */
NTSTATUS DrvClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	/* We're not serving IOCTL when we reach here because having a pending IOCTL won't let to close the handle so
	we use DbgPrint as it's safe here ! */
    /* 当我们到达这里时，我们不会处理挂起的 IOCTL 请求，因为存在挂起的 IOCTL 请求会阻止关闭句柄，
	所以我们在这里使用 DbgPrint 是安全的！ */
	DbgPrint("Terminating VMX...\n");
	// Terminating Vmx
    // 终止 Vmx
	HvTerminateVmx();

	DbgPrint("VMX Operation turned off successfully :)\n");
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/* Unsupported message for all other IRP_MJ_* handlers */
/* 对于所有其他的 IRP_MJ_* 处理程序，不支持的消息 */
NTSTATUS DrvUnsupported(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	LogWarning("This function is not supported :(");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/* Driver IOCTL Dispatcher*/
/* 驱动程序 IOCTL 调度程序 */
NTSTATUS DrvDispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION  IrpStack;
	PREGISTER_EVENT RegisterEvent;
	NTSTATUS    Status;

	if (AllowIOCTLFromUsermode)
	{

		// Here's the best place to see if there is any allocation pending to be allcated as we're in PASSIVE_LEVEL
        // 在这里是查看是否有待分配的分配请求的最佳位置，因为我们处于 PASSIVE_LEVEL。
		PoolManagerCheckAndPerformAllocation();

		IrpStack = IoGetCurrentIrpStackLocation(Irp);

		switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_REGISTER_EVENT:

			// First validate the parameters.
            // 首先验证参数。
			if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_REGISTER_EVENT || Irp->AssociatedIrp.SystemBuffer == NULL) {
				Status = STATUS_INVALID_PARAMETER;
				LogError("Invalid parameter to IOCTL Dispatcher.");
				break;
			}

			RegisterEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

			switch (RegisterEvent->Type) {
			case IRP_BASED:
				Status = LogRegisterIrpBasedNotification(DeviceObject, Irp);
				break;
			case EVENT_BASED:
				Status = LogRegisterEventBasedNotification(DeviceObject, Irp);
				break;
			default:
				ASSERTMSG("\tUnknow notification type from user-mode\n", FALSE);
				Status = STATUS_INVALID_PARAMETER;
				break;
			}
			break;
		case IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL:
			// Dis-allow new IOCTL
            // 不允许新的 IOCTL。
			AllowIOCTLFromUsermode = FALSE;
			// Send an immediate message, and we're no longer get new IRP
            // 发送一个即时消息，并且我们将不再接收新的 IRP。
			LogInfoImmediate("An immediate message recieved, we no longer recieve IRPs from user-mode ");
			Status = STATUS_SUCCESS;
			break;
		default:
			ASSERT(FALSE);  // should never hit this // 永远不应该执行到这里。
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
	}
	else
	{
		// We're no longer serve IOCTLL
        // 我们不再处理 IOCTL。
		Status = STATUS_SUCCESS;
	}
	if (Status != STATUS_PENDING) {
		Irp->IoStatus.Status = Status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return Status;
}
