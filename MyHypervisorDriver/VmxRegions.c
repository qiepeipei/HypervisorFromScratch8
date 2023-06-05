#include <ntddk.h>
#include <wdf.h>
#include <wdm.h>
#include "Msr.h"
#include "Vmx.h"
#include "Common.h"
#include "GlobalVariables.h"
#include "Dpc.h"
#include "InlineAsm.h"
#include "Vpid.h"
#include "Invept.h"

/* Allocates Vmx regions for all logical cores (Vmxon region and Vmcs region) */
/* 为所有逻辑核心分配Vmx区域（Vmxon区域和Vmcs区域） */
BOOLEAN VmxDpcBroadcastAllocateVmxonRegions(KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{

	int CurrentProcessorNumber = KeGetCurrentProcessorNumber();

	LogInfo("Allocating Vmx Regions for logical core %d", CurrentProcessorNumber);

	// Enabling VMX Operation
    // 启用VMX操作。
	AsmEnableVmxOperation();

	LogInfo("VMX-Operation Enabled Successfully");

	if (!VmxAllocateVmxonRegion(&GuestState[CurrentProcessorNumber]))
	{
		LogError("Error in allocating memory for Vmxon region");
		return FALSE;
	}
	if (!VmxAllocateVmcsRegion(&GuestState[CurrentProcessorNumber]))
	{
		LogError("Error in allocating memory for Vmcs region");
		return FALSE;
	}

	// Wait for all DPCs to synchronize at this point
    // 在此处等待所有的延迟处理程序（DPC）同步。
	KeSignalCallDpcSynchronize(SystemArgument2);

	// Mark the DPC as being complete
    // 将延迟处理程序（DPC）标记为已完成。
	KeSignalCallDpcDone(SystemArgument1);

	return TRUE;
}

/* Allocates Vmxon region and set the Revision ID based on IA32_VMX_BASIC_MSR */
/* 分配Vmxon区域并根据IA32_VMX_BASIC_MSR设置修订号（Revision ID） */
BOOLEAN VmxAllocateVmxonRegion(VIRTUAL_MACHINE_STATE* CurrentGuestState)
{
	PHYSICAL_ADDRESS PhysicalMax = { 0 };
	IA32_VMX_BASIC_MSR VmxBasicMsr = { 0 };
	int VmxonSize;
	int VmxonStatus;
	BYTE* VmxonRegion;
	UINT64 VmxonRegionPhysicalAddr;
	UINT64 AlignedVmxonRegion;
	UINT64 AlignedVmxonRegionPhysicalAddr;


	// at IRQL > DISPATCH_LEVEL memory allocation routines don't work
    // 在IRQL > DISPATCH_LEVEL的情况下，内存分配例程无法工作。
	if (KeGetCurrentIrql() > DISPATCH_LEVEL)
		KeRaiseIrqlToDpcLevel();

	PhysicalMax.QuadPart = MAXULONG64;

	VmxonSize = 2 * VMXON_SIZE;

	// Allocating a 4-KByte Contigous Memory region
    // 分配一个4KB的连续内存区域。
	VmxonRegion = MmAllocateContiguousMemory(VmxonSize + ALIGNMENT_PAGE_SIZE, PhysicalMax);

	if (VmxonRegion == NULL) {
		LogError("Couldn't Allocate Buffer for VMXON Region.");
		return FALSE;
	}

	VmxonRegionPhysicalAddr = VirtualAddressToPhysicalAddress(VmxonRegion);

	// zero-out memory 
	// 将内存清零。
	RtlSecureZeroMemory(VmxonRegion, VmxonSize + ALIGNMENT_PAGE_SIZE);


	AlignedVmxonRegion = (BYTE*)((ULONG_PTR)(VmxonRegion + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1));
	LogInfo("VMXON Region Address : %llx", AlignedVmxonRegion);

	// 4 kb >= buffers are aligned, just a double check to ensure if it's aligned
    // 4KB及以上的缓冲区已对齐，再次检查以确保其是否对齐。
	AlignedVmxonRegionPhysicalAddr = (BYTE*)((ULONG_PTR)(VmxonRegionPhysicalAddr + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1));
	LogInfo("VMXON Region Physical Address : %llx", AlignedVmxonRegionPhysicalAddr);

	// get IA32_VMX_BASIC_MSR RevisionId
    // 获取IA32_VMX_BASIC_MSR（VMX基本主机系统寄存器）的修订号（Revision ID）。
	VmxBasicMsr.All = __readmsr(MSR_IA32_VMX_BASIC);
	LogInfo("Revision Identifier (MSR_IA32_VMX_BASIC - MSR 0x480) : 0x%x", VmxBasicMsr.Fields.RevisionIdentifier);

	//Changing Revision Identifier
    // 修改修订标识符。
	*(UINT64*)AlignedVmxonRegion = VmxBasicMsr.Fields.RevisionIdentifier;

	// Execute Vmxon instruction
    // 执行Vmxon指令。
	VmxonStatus = __vmx_on(&AlignedVmxonRegionPhysicalAddr);
	if (VmxonStatus)
	{
		LogError("Executing Vmxon instruction failed with status : %d", VmxonStatus);
		return FALSE;
	}


	CurrentGuestState->VmxonRegionPhysicalAddress = AlignedVmxonRegionPhysicalAddr;

	// We save the allocated buffer (not the aligned buffer) because we want to free it in vmx termination
    // 我们保存分配的缓冲区（而不是对齐的缓冲区），因为我们希望在vmx终止时释放它。
	CurrentGuestState->VmxonRegionVirtualAddress = VmxonRegion;

	return TRUE;
}

/* Allocate Vmcs region and set the Revision ID based on IA32_VMX_BASIC_MSR */
/* 分配Vmcs区域并根据IA32_VMX_BASIC_MSR设置修订号（Revision ID） */
BOOLEAN VmxAllocateVmcsRegion(VIRTUAL_MACHINE_STATE* CurrentGuestState)
{
	PHYSICAL_ADDRESS PhysicalMax = { 0 };
	int VmcsSize;
	BYTE* VmcsRegion;
	UINT64 VmcsPhysicalAddr;
	UINT64 AlignedVmcsRegion;
	UINT64 AlignedVmcsRegionPhysicalAddr;
	IA32_VMX_BASIC_MSR VmxBasicMsr = { 0 };


	// at IRQL > DISPATCH_LEVEL memory allocation routines don't work
    // 在IRQL > DISPATCH_LEVEL的情况下，内存分配例程无法工作。
	if (KeGetCurrentIrql() > DISPATCH_LEVEL)
		KeRaiseIrqlToDpcLevel();

	PhysicalMax.QuadPart = MAXULONG64;

	VmcsSize = 2 * VMCS_SIZE;
	VmcsRegion = MmAllocateContiguousMemory(VmcsSize + ALIGNMENT_PAGE_SIZE, PhysicalMax);  // Allocating a 4-KByte Contigous Memory region

	if (VmcsRegion == NULL) {
		LogError("Couldn't Allocate Buffer for VMCS Region.");
		return FALSE;
	}
	RtlSecureZeroMemory(VmcsRegion, VmcsSize + ALIGNMENT_PAGE_SIZE);

	VmcsPhysicalAddr = VirtualAddressToPhysicalAddress(VmcsRegion);

	AlignedVmcsRegion = (BYTE*)((ULONG_PTR)(VmcsRegion + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1));
	LogInfo("VMCS Region Address : %llx", AlignedVmcsRegion);

	AlignedVmcsRegionPhysicalAddr = (BYTE*)((ULONG_PTR)(VmcsPhysicalAddr + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1));
	LogInfo("VMCS Region Physical Address : %llx", AlignedVmcsRegionPhysicalAddr);

	// get IA32_VMX_BASIC_MSR RevisionId
    // 获取IA32_VMX_BASIC_MSR（VMX基本主机系统寄存器）的修订号（Revision ID）。
	VmxBasicMsr.All = __readmsr(MSR_IA32_VMX_BASIC);
	LogInfo("Revision Identifier (MSR_IA32_VMX_BASIC - MSR 0x480) : 0x%x", VmxBasicMsr.Fields.RevisionIdentifier);


	//Changing Revision Identifier
    // 修改修订标识符。
	*(UINT64*)AlignedVmcsRegion = VmxBasicMsr.Fields.RevisionIdentifier;

	CurrentGuestState->VmcsRegionPhysicalAddress = AlignedVmcsRegionPhysicalAddr;
	// We save the allocated buffer (not the aligned buffer) because we want to free it in vmx termination
    // 我们保存分配的缓冲区（而不是对齐的缓冲区），因为我们希望在vmx终止时释放它。
	CurrentGuestState->VmcsRegionVirtualAddress = VmcsRegion;

	return TRUE;
}

/* Allocate VMM Stack */
/* 分配VMM（虚拟机监控程序）栈 */
BOOLEAN VmxAllocateVmmStack(INT ProcessorID)
{
	UINT64 VmmStack;

	// Allocate stack for the VM Exit Handler.
    // 为VM Exit Handler分配栈。
	VmmStack = ExAllocatePoolWithTag(NonPagedPool, VMM_STACK_SIZE, POOLTAG);
	GuestState[ProcessorID].VmmStack = VmmStack;

	if (GuestState[ProcessorID].VmmStack == NULL)
	{
		LogError("Insufficient memory in allocationg Vmm stack");
		return FALSE;
	}
	RtlZeroMemory(GuestState[ProcessorID].VmmStack, VMM_STACK_SIZE);

	LogInfo("Vmm Stack for logical processor : 0x%llx", GuestState[ProcessorID].VmmStack);

	return TRUE;
}

/* Allocate a buffer forr Msr Bitmap */
/* 分配用于Msr位图的缓冲区 */
BOOLEAN VmxAllocateMsrBitmap(INT ProcessorID)
{
	// Allocate memory for MSRBitMap
    // 为MSR位图分配内存。
	GuestState[ProcessorID].MsrBitmapVirtualAddress = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG);  // should be aligned

	if (GuestState[ProcessorID].MsrBitmapVirtualAddress == NULL)
	{
		LogError("Insufficient memory in allocationg Msr bitmaps");
		return FALSE;
	}
	RtlZeroMemory(GuestState[ProcessorID].MsrBitmapVirtualAddress, PAGE_SIZE);

	GuestState[ProcessorID].MsrBitmapPhysicalAddress = VirtualAddressToPhysicalAddress(GuestState[ProcessorID].MsrBitmapVirtualAddress);

	LogInfo("Msr Bitmap Virtual Address : 0x%llx", GuestState[ProcessorID].MsrBitmapVirtualAddress);
	LogInfo("Msr Bitmap Physical Address : 0x%llx", GuestState[ProcessorID].MsrBitmapPhysicalAddress);

	// (Uncomment if you want to break on RDMSR and WRMSR to a special MSR Register)
    // （如果您想在RDMSR和WRMSR到特殊MSR寄存器时中断，请取消注释）
	/*
	if (HvSetMsrBitmap(0xc0000082, ProcessorID, TRUE, TRUE))
	{
		LogError("Invalid parameters sent to the HvSetMsrBitmap function");
		return FALSE;
	}
	*/

	return TRUE;
}