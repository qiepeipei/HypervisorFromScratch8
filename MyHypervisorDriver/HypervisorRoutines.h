#pragma once
#include "Msr.h"
#include "Vmx.h"

/*
   This file contains the headers for Hypervisor Routines which have to be called by external codes,
		DO NOT DIRECTLY CALL VMX FUNCTIONS,
			instead use these routines.
*/
/*
本文件包含外部代码需要调用的虚拟化程序例程的头文件，
不要直接调用VMX函数，
而是使用这些例程。
*/

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

// Detect whether Vmx is supported or not
// 检测是否支持VMX（Virtual Machine Extensions）
BOOLEAN HvIsVmxSupported();

// Initialize Vmx 
// 初始化VMX（Virtual Machine Extensions）
BOOLEAN HvVmxInitialize();

// Allocates Vmx regions for all logical cores (Vmxon region and Vmcs region)
// 为所有逻辑核心分配VMX区域（Vmxon区域和Vmcs区域）
BOOLEAN VmxDpcBroadcastAllocateVmxonRegions(KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

// Set Guest Selector Registers
// 设置客户选择器寄存器（Guest Selector Registers）
BOOLEAN HvSetGuestSelector(PVOID GdtBase, ULONG SegmentRegister, USHORT Selector);

// Get Segment Descriptor
// 获取段描述符（Segment Descriptor）
BOOLEAN HvGetSegmentDescriptor(PSEGMENT_SELECTOR SegmentSelector, USHORT Selector, PUCHAR GdtBase);

// Set Msr Bitmap
// 设置MSR位图（Msr Bitmap）
BOOLEAN HvSetMsrBitmap(ULONG64 Msr, INT ProcessorID, BOOLEAN ReadDetection, BOOLEAN WriteDetection);

// Returns the Cpu Based and Secondary Processor Based Controls and other controls based on hardware support 
// 返回基于CPU支持的控制（Cpu Based Controls）和基于辅助处理器支持的控制（Secondary Processor Based Controls）以及其他基于硬件支持的控制信息
ULONG HvAdjustControls(ULONG Ctl, ULONG Msr);

// Notify all cores about EPT Invalidation
// 通知所有核心关于EPT失效的情况
VOID HvNotifyAllToInvalidateEpt();

// Handle Cpuid
// 处理Cpuid指令
VOID HvHandleCpuid(PGUEST_REGS RegistersState);

// Fill guest selector data
// 填充客户选择器数据
VOID HvFillGuestSelectorData(PVOID GdtBase, ULONG SegmentRegister, USHORT Selector);

// Handle Guest's Control Registers Access
// 处理客户控制寄存器的访问
VOID HvHandleControlRegisterAccess(PGUEST_REGS GuestState);

// Handle Guest's Msr read
// 处理客户MSR读取
VOID HvHandleMsrRead(PGUEST_REGS GuestRegs);

// Handle Guest's Msr write
// 处理客户MSR写入
VOID HvHandleMsrWrite(PGUEST_REGS GuestRegs);

// Resume GUEST_RIP to next instruction
// 恢复GUEST_RIP到下一条指令
VOID HvResumeToNextInstruction();

// Invalidate EPT using Vmcall (should be called from Vmx non root mode)
// 使用Vmcall来使EPT失效（应该从非根模式的Vmx中调用）
VOID HvInvalidateEptByVmcall(UINT64 Context);

// The broadcast function which initialize the guest
// 初始化客户的广播函数
VOID HvDpcBroadcastInitializeGuest(KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

// The broadcast function which terminate the guest
// 终止客户的广播函数
VOID HvDpcBroadcastTerminateGuest(KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

// Terminate Vmx on all logical cores.
// 在所有逻辑核心上终止Vmx（虚拟机扩展）
VOID HvTerminateVmx();

// Set or unset the monitor trap flags
// 设置或取消监视陷阱标志（monitor trap flag）
VOID HvSetMonitorTrapFlag(BOOLEAN Set);

// Returns the stack pointer, to change in the case of Vmxoff 
// 返回堆栈指针，在Vmxoff情况下进行更改
UINT64 HvReturnStackPointerForVmxoff();

// Returns the instruction pointer, to change in the case of Vmxoff 
// 返回指令指针，在Vmxoff情况下进行更改
UINT64 HvReturnInstructionPointerForVmxoff();

// Reset GDTR/IDTR and other old when you do vmxoff as the patchguard will detect them left modified
// 在执行vmxoff时重置GDTR/IDTR和其他旧值，因为PatchGuard会检测到它们被修改了
VOID HvRestoreRegisters();

// Remove single hook from the hooked pages list and invalidate TLB 
// 从钩住的页面列表中移除单个钩住并使TLB失效
BOOLEAN HvPerformPageUnHookSinglePage(UINT64 VirtualAddress);

// Remove all hooks from the hooked pages list and invalidate TLB
// 从钩住的页面列表中移除所有钩住并使TLB失效
VOID HvPerformPageUnHookAllPages();
