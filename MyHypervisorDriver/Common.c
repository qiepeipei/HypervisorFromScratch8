#include <ntddk.h>
#include <wdf.h>
#include "Msr.h"
#include "Common.h"
#include "Vmx.h"

/* Power function in order to computer address for MSR bitmaps */
/* 用于计算MSR位图地址的幂函数 */
int MathPower(int Base, int Exp) {

	int result;

	result = 1;

	for (;;)
	{
		if (Exp & 1)
		{
			result *= Base;
		}
		Exp >>= 1;
		if (!Exp)
		{
			break;
		}
		Base *= Base;
	}
	return result;
}

// This function is deprecated as we want to supporrt more than 32 processors.
/* Broadcast a function to all logical cores */
// 此函数已弃用，因为我们希望支持超过32个处理器。
/* 将函数广播到所有逻辑核心 */
BOOLEAN BroadcastToProcessors(ULONG ProcessorNumber, RunOnLogicalCoreFunc Routine)
{

	KIRQL OldIrql;

	KeSetSystemAffinityThread((KAFFINITY)(1 << ProcessorNumber));

	OldIrql = KeRaiseIrqlToDpcLevel();

	Routine(ProcessorNumber);

	KeLowerIrql(OldIrql);

	KeRevertToUserAffinityThread();

	return TRUE;
}

// Note : Because of deadlock and synchronization problem, no longer use this instead use Vmcall with VMCALL_VMXOFF
/* Broadcast a 0x41414141 - 0x42424242 message to CPUID Handler of all logical cores in order to turn off VMX in VMX root-mode */
/*BOOLEAN BroadcastToProcessorsToTerminateVmx(ULONG ProcessorNumber)
{
	KIRQL OldIrql;

	KeSetSystemAffinityThread((KAFFINITY)(1 << ProcessorNumber));

	OldIrql = KeRaiseIrqlToDpcLevel();

	INT32 cpu_info[4];
	__cpuidex(cpu_info, 0x41414141, 0x42424242);

	KeLowerIrql(OldIrql);

	KeRevertToUserAffinityThread();

	return TRUE;
}
*/

/* Set Bits for a special address (used on MSR Bitmaps) */
/* 设置特定地址的位（用于MSR位图） */
void SetBit(PVOID Addr, UINT64 bit, BOOLEAN Set) {

	UINT64 byte;
	UINT64 temp;
	UINT64 n;
	BYTE* Addr2;

	byte = bit / 8;
	temp = bit % 8;
	n = 7 - temp;

	Addr2 = Addr;

	if (Set)
	{
		Addr2[byte] |= (1 << n);
	}
	else
	{
		Addr2[byte] &= ~(1 << n);
	}
}

/* Get Bits of a special address (used on MSR Bitmaps) */
/* 获取特定地址的位（用于MSR位图） */
void GetBit(PVOID Addr, UINT64 bit) {

	UINT64 byte, k;
	BYTE* Addr2;

	byte = 0;
	k = 0;
	byte = bit / 8;
	k = 7 - bit % 8;

	Addr2 = Addr;

	return Addr2[byte] & (1 << k);
}

/* Converts Virtual Address to Physical Address */
/* 将虚拟地址转换为物理地址 */
UINT64 VirtualAddressToPhysicalAddress(PVOID VirtualAddress)
{
	return MmGetPhysicalAddress(VirtualAddress).QuadPart;
}

/* Converts Physical Address to Virtual Address */
/* 将物理地址转换为虚拟地址 */
UINT64 PhysicalAddressToVirtualAddress(UINT64 PhysicalAddress)
{
	PHYSICAL_ADDRESS PhysicalAddr;
	PhysicalAddr.QuadPart = PhysicalAddress;

	return MmGetVirtualForPhysical(PhysicalAddr);
}

/* Find cr3 of system process*/
/* 查找系统进程的CR3值 */
UINT64 FindSystemDirectoryTableBase()
{
	// Return CR3 of the system process.
    // 返回系统进程的CR3值。
	NT_KPROCESS* SystemProcess = (NT_KPROCESS*)(PsInitialSystemProcess);
	return SystemProcess->DirectoryTableBase;
}