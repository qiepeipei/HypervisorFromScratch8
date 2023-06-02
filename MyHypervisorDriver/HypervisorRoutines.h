#pragma once
#include "Msr.h"
#include "Vmx.h"

/*
   This file contains the headers for Hypervisor Routines which have to be called by external codes,
		DO NOT DIRECTLY CALL VMX FUNCTIONS,
			instead use these routines.
*/
/*
���ļ������ⲿ������Ҫ���õ����⻯�������̵�ͷ�ļ���
��Ҫֱ�ӵ���VMX������
����ʹ����Щ���̡�
*/

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

// Detect whether Vmx is supported or not
// ����Ƿ�֧��VMX��Virtual Machine Extensions��
BOOLEAN HvIsVmxSupported();

// Initialize Vmx 
// ��ʼ��VMX��Virtual Machine Extensions��
BOOLEAN HvVmxInitialize();

// Allocates Vmx regions for all logical cores (Vmxon region and Vmcs region)
// Ϊ�����߼����ķ���VMX����Vmxon�����Vmcs����
BOOLEAN VmxDpcBroadcastAllocateVmxonRegions(KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

// Set Guest Selector Registers
// ���ÿͻ�ѡ�����Ĵ�����Guest Selector Registers��
BOOLEAN HvSetGuestSelector(PVOID GdtBase, ULONG SegmentRegister, USHORT Selector);

// Get Segment Descriptor
// ��ȡ����������Segment Descriptor��
BOOLEAN HvGetSegmentDescriptor(PSEGMENT_SELECTOR SegmentSelector, USHORT Selector, PUCHAR GdtBase);

// Set Msr Bitmap
// ����MSRλͼ��Msr Bitmap��
BOOLEAN HvSetMsrBitmap(ULONG64 Msr, INT ProcessorID, BOOLEAN ReadDetection, BOOLEAN WriteDetection);

// Returns the Cpu Based and Secondary Processor Based Controls and other controls based on hardware support 
// ���ػ���CPU֧�ֵĿ��ƣ�Cpu Based Controls���ͻ��ڸ���������֧�ֵĿ��ƣ�Secondary Processor Based Controls���Լ���������Ӳ��֧�ֵĿ�����Ϣ
ULONG HvAdjustControls(ULONG Ctl, ULONG Msr);

// Notify all cores about EPT Invalidation
// ֪ͨ���к��Ĺ���EPTʧЧ�����
VOID HvNotifyAllToInvalidateEpt();

// Handle Cpuid
// ����Cpuidָ��
VOID HvHandleCpuid(PGUEST_REGS RegistersState);

// Fill guest selector data
// ���ͻ�ѡ��������
VOID HvFillGuestSelectorData(PVOID GdtBase, ULONG SegmentRegister, USHORT Selector);

// Handle Guest's Control Registers Access
// ����ͻ����ƼĴ����ķ���
VOID HvHandleControlRegisterAccess(PGUEST_REGS GuestState);

// Handle Guest's Msr read
// ����ͻ�MSR��ȡ
VOID HvHandleMsrRead(PGUEST_REGS GuestRegs);

// Handle Guest's Msr write
// ����ͻ�MSRд��
VOID HvHandleMsrWrite(PGUEST_REGS GuestRegs);

// Resume GUEST_RIP to next instruction
// �ָ�GUEST_RIP����һ��ָ��
VOID HvResumeToNextInstruction();

// Invalidate EPT using Vmcall (should be called from Vmx non root mode)
// ʹ��Vmcall��ʹEPTʧЧ��Ӧ�ôӷǸ�ģʽ��Vmx�е��ã�
VOID HvInvalidateEptByVmcall(UINT64 Context);

// The broadcast function which initialize the guest
// ��ʼ���ͻ��Ĺ㲥����
VOID HvDpcBroadcastInitializeGuest(KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

// The broadcast function which terminate the guest
// ��ֹ�ͻ��Ĺ㲥����
VOID HvDpcBroadcastTerminateGuest(KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

// Terminate Vmx on all logical cores.
// �������߼���������ֹVmx���������չ��
VOID HvTerminateVmx();

// Set or unset the monitor trap flags
// ���û�ȡ�����������־��monitor trap flag��
VOID HvSetMonitorTrapFlag(BOOLEAN Set);

// Returns the stack pointer, to change in the case of Vmxoff 
// ���ض�ջָ�룬��Vmxoff����½��и���
UINT64 HvReturnStackPointerForVmxoff();

// Returns the instruction pointer, to change in the case of Vmxoff 
// ����ָ��ָ�룬��Vmxoff����½��и���
UINT64 HvReturnInstructionPointerForVmxoff();

// Reset GDTR/IDTR and other old when you do vmxoff as the patchguard will detect them left modified
// ��ִ��vmxoffʱ����GDTR/IDTR��������ֵ����ΪPatchGuard���⵽���Ǳ��޸���
VOID HvRestoreRegisters();

// Remove single hook from the hooked pages list and invalidate TLB 
// �ӹ�ס��ҳ���б����Ƴ�������ס��ʹTLBʧЧ
BOOLEAN HvPerformPageUnHookSinglePage(UINT64 VirtualAddress);

// Remove all hooks from the hooked pages list and invalidate TLB
// �ӹ�ס��ҳ���б����Ƴ����й�ס��ʹTLBʧЧ
VOID HvPerformPageUnHookAllPages();
