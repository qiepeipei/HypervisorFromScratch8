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
/* ���������������µ�������������� */
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
    // ��ʼ�� WPP ����
	WPP_INIT_TRACING(DriverObject, RegistryPath);

#if !UseDbgPrintInsteadOfUsermodeMessageTracking 
	if (!LogInitialize())
	{
		DbgPrint("[*] Log buffer is not initialized !\n");
		DbgBreakPoint();
	}
#endif

	// Opt-in to using non-executable pool memory on Windows 8 and later.
    // �� Windows 8 �����߰汾��ѡ��ʹ�÷ǿ�ִ�г��ڴ档
	// https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

	/////////////// we allocate virtual machine here because we want to use its state (vmx-root or vmx non-root) in logs \\\\\\\\\\\\\\\
	/////////////// ������������������Ϊ����ϣ������־��ʹ����״̬��vmx-root �� vmx non-root���� \\\\\\\\

	ProcessorCount = KeQueryActiveProcessorCount(0);

	// Allocate global variable to hold Guest(s) state
    // ����ȫ�ֱ����Ա����������״̬
	GuestState = ExAllocatePoolWithTag(NonPagedPool, sizeof(VIRTUAL_MACHINE_STATE) * ProcessorCount, POOLTAG);

	if (!GuestState)
	{
		// we use DbgPrint as the vmx-root or non-root is not initialized
        // ���� vmx-root �� non-root δ��ʼ��������ʹ�� DbgPrint��
		DbgPrint("Insufficient memory\n");
		DbgBreakPoint();
		return FALSE;
	}

	// Zero memory
    // �����ڴ�
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
    // �����û����������ʷ�����
	DeviceObject->Flags |= DO_BUFFERED_IO;

	ASSERT(NT_SUCCESS(Ntstatus));
	return Ntstatus;
}

/* Run in the case of driver unload to unregister the devices */
/* ����������ж�ص���������У���ע���豸 */
VOID DrvUnload(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING DosDeviceName;


	RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\MyHypervisorDevice");
	IoDeleteSymbolicLink(&DosDeviceName);
	IoDeleteDevice(DriverObject->DeviceObject);

	DbgPrint("Hypervisor From Scratch's driver unloaded\n");

#if !UseDbgPrintInsteadOfUsermodeMessageTracking 
	// Uinitialize log buffer
    // ����ʼ����־������
	DbgPrint("Uinitializing logs\n");
	LogUnInitialize();
#endif
	// Free GuestState 
	// �ͷ� GuestState ��Դ
	ExFreePoolWithTag(GuestState, POOLTAG);

	// Stop the tracing
    // ֹͣ����
	WPP_CLEANUP(DriverObject);

}


/* IRP_MJ_CREATE Function handler*/
/* IRP_MJ_CREATE ����������� */
NTSTATUS DrvCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	int ProcessorCount;

	// Allow to server IOCTL 
	// ������� IOCTL ����
	AllowIOCTLFromUsermode = TRUE;

	LogInfo("Hypervisor From Scratch Started...");


	/* We have to zero the GuestState again as we want to support multiple initialization by CreateFile */
    /* ��������Ҫ֧��ͨ�� CreateFile ���ж�γ�ʼ�������Ǳ����ٴν� GuestState ���� */
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
/* IRP_MJ_READ ����������� */
NTSTATUS DrvRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	LogWarning("Not implemented yet :(");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/* IRP_MJ_WRITE Function handler*/
/* IRP_MJ_WRITE ����������� */
NTSTATUS DrvWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	LogWarning("Not implemented yet :(");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/* IRP_MJ_CLOSE Function handler*/
/* IRP_MJ_CLOSE ����������� */
NTSTATUS DrvClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	/* We're not serving IOCTL when we reach here because having a pending IOCTL won't let to close the handle so
	we use DbgPrint as it's safe here ! */
    /* �����ǵ�������ʱ�����ǲ��ᴦ������ IOCTL ������Ϊ���ڹ���� IOCTL �������ֹ�رվ����
	��������������ʹ�� DbgPrint �ǰ�ȫ�ģ� */
	DbgPrint("Terminating VMX...\n");
	// Terminating Vmx
    // ��ֹ Vmx
	HvTerminateVmx();

	DbgPrint("VMX Operation turned off successfully :)\n");
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/* Unsupported message for all other IRP_MJ_* handlers */
/* �������������� IRP_MJ_* ������򣬲�֧�ֵ���Ϣ */
NTSTATUS DrvUnsupported(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	LogWarning("This function is not supported :(");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/* Driver IOCTL Dispatcher*/
/* �������� IOCTL ���ȳ��� */
NTSTATUS DrvDispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION  IrpStack;
	PREGISTER_EVENT RegisterEvent;
	NTSTATUS    Status;

	if (AllowIOCTLFromUsermode)
	{

		// Here's the best place to see if there is any allocation pending to be allcated as we're in PASSIVE_LEVEL
        // �������ǲ鿴�Ƿ��д�����ķ�����������λ�ã���Ϊ���Ǵ��� PASSIVE_LEVEL��
		PoolManagerCheckAndPerformAllocation();

		IrpStack = IoGetCurrentIrpStackLocation(Irp);

		switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_REGISTER_EVENT:

			// First validate the parameters.
            // ������֤������
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
            // �������µ� IOCTL��
			AllowIOCTLFromUsermode = FALSE;
			// Send an immediate message, and we're no longer get new IRP
            // ����һ����ʱ��Ϣ���������ǽ����ٽ����µ� IRP��
			LogInfoImmediate("An immediate message recieved, we no longer recieve IRPs from user-mode ");
			Status = STATUS_SUCCESS;
			break;
		default:
			ASSERT(FALSE);  // should never hit this // ��Զ��Ӧ��ִ�е����
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
	}
	else
	{
		// We're no longer serve IOCTLL
        // ���ǲ��ٴ��� IOCTL��
		Status = STATUS_SUCCESS;
	}
	if (Status != STATUS_PENDING) {
		Irp->IoStatus.Status = Status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return Status;
}
