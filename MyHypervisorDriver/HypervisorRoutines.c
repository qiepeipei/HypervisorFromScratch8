//  This file describe the routines in Hypervisor
#include "Msr.h"
#include "Vmx.h"
#include "Common.h"
#include "GlobalVariables.h"
#include "HypervisorRoutines.h"
#include "Invept.h"
#include "InlineAsm.h"
#include "Vpid.h"
#include "Vmcall.h"
#include "Dpc.h"

/* Initialize Vmx */
/* 初始化Vmx（虚拟化扩展） */
BOOLEAN HvVmxInitialize()
{

	int LogicalProcessorsCount;
	IA32_VMX_BASIC_MSR VmxBasicMsr = { 0 };

	/*** Start Virtualizing Current System ***/
    /*** 开始对当前系统进行虚拟化 ***/

	// Initiating EPTP and VMX
    // 初始化EPTP（扩展页表指针）和VMX（虚拟机扩展）
	if (!VmxInitializer())
	{
		// there was error somewhere in initializing
        // 在初始化过程中发生了错误
		return FALSE;
	}

	LogicalProcessorsCount = KeQueryActiveProcessorCount(0);

	for (size_t ProcessorID = 0; ProcessorID < LogicalProcessorsCount; ProcessorID++)
	{
		/*** Launching VM for Test (in the all logical processor) ***/
        /*** 启动虚拟机进行测试（在所有逻辑处理器上） ***/

		//Allocating VMM Stack
        //分配VMM堆栈
		if (!VmxAllocateVmmStack(ProcessorID))
		{
			// Some error in allocating Vmm Stack
            //在分配VMM堆栈时发生了错误
			return FALSE;
		}

		// Allocating MSR Bit 
		// 分配MSR位
		if (!VmxAllocateMsrBitmap(ProcessorID))
		{
			// Some error in allocating Msr Bitmaps
            // 在分配MSR位图时发生了错误
			return FALSE;
		}

		/*** This function is deprecated as we want to supporrt more than 32 processors. ***/
        /*** 此函数已被弃用，因为我们希望支持超过32个处理器。***/
		// BroadcastToProcessors(ProcessorID, AsmVmxSaveState);

	}

	// As we want to support more than 32 processor (64 logical-core) we let windows execute our routine for us
    // 由于我们希望支持超过32个处理器（64个逻辑核心），我们让Windows代为执行我们的例程
	KeGenericCallDpc(HvDpcBroadcastInitializeGuest, 0x0);

	// Check if everything is ok then return true otherwise false
    // 检查是否一切正常，然后返回true，否则返回false
	if (AsmVmxVmcall(VMCALL_TEST, 0x22, 0x333, 0x4444) == STATUS_SUCCESS)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/* Check whether VMX Feature is supported or not */
/* 检查是否支持VMX功能 */
BOOLEAN HvIsVmxSupported()
{
	CPUID Data = { 0 };
	IA32_FEATURE_CONTROL_MSR FeatureControlMsr = { 0 };

	// VMX bit
	__cpuid((int*)&Data, 1);
	if ((Data.ecx & (1 << 5)) == 0)
		return FALSE;

	FeatureControlMsr.All = __readmsr(MSR_IA32_FEATURE_CONTROL);

	// BIOS lock check
    // BIOS锁定检查
	if (FeatureControlMsr.Fields.Lock == 0)
	{
		FeatureControlMsr.Fields.Lock = TRUE;
		FeatureControlMsr.Fields.EnableVmxon = TRUE;
		__writemsr(MSR_IA32_FEATURE_CONTROL, FeatureControlMsr.All);
	}
	else if (FeatureControlMsr.Fields.EnableVmxon == FALSE)
	{
		LogError("Intel VMX feature is locked in BIOS");
		return FALSE;
	}

	return TRUE;
}

/* Returns the Cpu Based and Secondary Processor Based Controls and other controls based on hardware support */
/* 返回基于硬件支持的CPU Based和Secondary Processor Based控制以及其他控制 */
ULONG HvAdjustControls(ULONG Ctl, ULONG Msr)
{
	MSR MsrValue = { 0 };

	MsrValue.Content = __readmsr(Msr);
	Ctl &= MsrValue.High;     /* bit == 0 in high word ==> must be zero */ /* 高字中的位等于0 ==> 必须为零 */
	Ctl |= MsrValue.Low;      /* bit == 1 in low word  ==> must be one  */ /* 低字中的位等于1 ==> 必须为一 */
	return Ctl;
}


/* Set guest's selector registers */
/* 设置客户机的选择器寄存器 */
BOOLEAN HvSetGuestSelector(PVOID GdtBase, ULONG SegmentRegister, USHORT Selector)
{
	SEGMENT_SELECTOR SegmentSelector = { 0 };
	ULONG AccessRights;

	HvGetSegmentDescriptor(&SegmentSelector, Selector, GdtBase);
	AccessRights = ((PUCHAR)&SegmentSelector.ATTRIBUTES)[0] + (((PUCHAR)&SegmentSelector.ATTRIBUTES)[1] << 12);

	if (!Selector)
		AccessRights |= 0x10000;

	__vmx_vmwrite(GUEST_ES_SELECTOR + SegmentRegister * 2, Selector);
	__vmx_vmwrite(GUEST_ES_LIMIT + SegmentRegister * 2, SegmentSelector.LIMIT);
	__vmx_vmwrite(GUEST_ES_AR_BYTES + SegmentRegister * 2, AccessRights);
	__vmx_vmwrite(GUEST_ES_BASE + SegmentRegister * 2, SegmentSelector.BASE);

	return TRUE;
}


/* Get Segment Descriptor */
/* 获取段描述符 */
BOOLEAN HvGetSegmentDescriptor(PSEGMENT_SELECTOR SegmentSelector, USHORT Selector, PUCHAR GdtBase)
{
	PSEGMENT_DESCRIPTOR SegDesc;

	if (!SegmentSelector)
		return FALSE;

	if (Selector & 0x4) {
		return FALSE;
	}

	SegDesc = (PSEGMENT_DESCRIPTOR)((PUCHAR)GdtBase + (Selector & ~0x7));

	SegmentSelector->SEL = Selector;
	SegmentSelector->BASE = SegDesc->BASE0 | SegDesc->BASE1 << 16 | SegDesc->BASE2 << 24;
	SegmentSelector->LIMIT = SegDesc->LIMIT0 | (SegDesc->LIMIT1ATTR1 & 0xf) << 16;
	SegmentSelector->ATTRIBUTES.UCHARs = SegDesc->ATTR0 | (SegDesc->LIMIT1ATTR1 & 0xf0) << 4;

	if (!(SegDesc->ATTR0 & 0x10)) { // LA_ACCESSED
		ULONG64 tmp;
		// this is a TSS or callgate etc, save the base high part
        // 这是一个TSS或者调用门等，保存基地址的高位部分
		tmp = (*(PULONG64)((PUCHAR)SegDesc + 8));
		SegmentSelector->BASE = (SegmentSelector->BASE & 0xffffffff) | (tmp << 32);
	}

	if (SegmentSelector->ATTRIBUTES.Fields.G) {
		// 4096-bit granularity is enabled for this segment, scale the limit
        // 此段启用了4096位粒度，调整限制值
		SegmentSelector->LIMIT = (SegmentSelector->LIMIT << 12) + 0xfff;
	}

	return TRUE;
}


/* Handle Cpuid Vmexits*/
/* 处理CPUID VMexit */
VOID HvHandleCpuid(PGUEST_REGS RegistersState)
{
	INT32 cpu_info[4];
	ULONG Mode = 0;


	// Otherwise, issue the CPUID to the logical processor based on the indexes
	// on the VP's GPRs.
    // 根据VP的GPR中的索引向逻辑处理器发出CPUID指令。
	__cpuidex(cpu_info, (INT32)RegistersState->rax, (INT32)RegistersState->rcx);

	// Check if this was CPUID 1h, which is the features request.
    // 检查是否为CPUID 1h，即功能请求。
	if (RegistersState->rax == CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS)
	{

		// Set the Hypervisor Present-bit in RCX, which Intel and AMD have both
		// reserved for this indication.
        // 设置RCX中的Hypervisor Present位，这是Intel和AMD都为此指示保留的位。
		cpu_info[2] |= HYPERV_HYPERVISOR_PRESENT_BIT;
	}
	else if (RegistersState->rax == CPUID_HV_VENDOR_AND_MAX_FUNCTIONS)
	{

		// Return a maximum supported hypervisor CPUID leaf range and a vendor
		// ID signature as required by the spec.
        // 根据规范要求，返回最大支持的Hypervisor CPUID叶子范围和厂商ID签名。

		cpu_info[0] = HYPERV_CPUID_INTERFACE;
		cpu_info[1] = 'rFvH';  // "[H]yper[v]isor [Fr]o[m] [Scratch] = HvFrmScratch"
		cpu_info[2] = 'rcSm';
		cpu_info[3] = 'hcta';
	}
	else if (RegistersState->rax == HYPERV_CPUID_INTERFACE)
	{
		// Return our interface identifier
        // 返回我们的接口标识符
		//cpu_info[0] = 'HVFS'; // [H]yper[V]isor [F]rom [S]cratch 

		// Return non Hv#1 value. This indicate that our hypervisor does NOT
		// conform to the Microsoft hypervisor interface.
        // 返回非Hv#1的值。这表明我们的hypervisor不符合Microsoft hypervisor接口。

		cpu_info[0] = '0#vH';  // Hv#0
		cpu_info[1] = cpu_info[2] = cpu_info[3] = 0;

	}

	// Copy the values from the logical processor registers into the VP GPRs.
    // 将逻辑处理器寄存器中的值复制到VP的GPR中。
	RegistersState->rax = cpu_info[0];
	RegistersState->rbx = cpu_info[1];
	RegistersState->rcx = cpu_info[2];
	RegistersState->rdx = cpu_info[3];

}

/* Handles Guest Access to control registers */
/* 处理客户机对控制寄存器的访问 */
VOID HvHandleControlRegisterAccess(PGUEST_REGS GuestState)
{
	ULONG ExitQualification = 0;
	PMOV_CR_QUALIFICATION CrExitQualification;
	PULONG64 RegPtr;
	INT64 GuestRsp = 0;

	__vmx_vmread(EXIT_QUALIFICATION, &ExitQualification);

	CrExitQualification = (PMOV_CR_QUALIFICATION)&ExitQualification;

	RegPtr = (PULONG64)&GuestState->rax + CrExitQualification->Fields.Register;

	/* Because its RSP and as we didn't save RSP correctly (because of pushes) so we have make it points to the GUEST_RSP */
    /* 因为这是RSP，并且我们没有正确保存RSP（由于推送操作），所以我们必须使其指向GUEST_RSP */
	if (CrExitQualification->Fields.Register == 4)
	{
		__vmx_vmread(GUEST_RSP, &GuestRsp);
		*RegPtr = GuestRsp;
	}

	switch (CrExitQualification->Fields.AccessType)
	{
	case TYPE_MOV_TO_CR:
	{
		switch (CrExitQualification->Fields.ControlRegister)
		{
		case 0:
			__vmx_vmwrite(GUEST_CR0, *RegPtr);
			__vmx_vmwrite(CR0_READ_SHADOW, *RegPtr);
			break;
		case 3:
			__vmx_vmwrite(GUEST_CR3, (*RegPtr & ~(1ULL << 63)));
			// InveptSingleContext(EptState->EptPointer.Flags); (changed, look for "Update 1" at the 8th part for more detail)
            // InveptSingleContext(EptState->EptPointer.Flags);（已更改，请查看第8部分的“更新1”以获取更多详细信息）
			InvvpidSingleContext(VPID_TAG);
			break;
		case 4:
			__vmx_vmwrite(GUEST_CR4, *RegPtr);
			__vmx_vmwrite(CR4_READ_SHADOW, *RegPtr);

			break;
		default:
			LogWarning("Unsupported register %d in handling control registers access", CrExitQualification->Fields.ControlRegister);
			break;
		}
	}
	break;

	case TYPE_MOV_FROM_CR:
	{
		switch (CrExitQualification->Fields.ControlRegister)
		{
		case 0:
			__vmx_vmread(GUEST_CR0, RegPtr);
			break;
		case 3:
			__vmx_vmread(GUEST_CR3, RegPtr);
			break;
		case 4:
			__vmx_vmread(GUEST_CR4, RegPtr);
			break;
		default:
			LogWarning("Unsupported register %d in handling control registers access", CrExitQualification->Fields.ControlRegister);
			break;
		}
	}
	break;

	default:
		LogWarning("Unsupported operation %d in handling control registers access", CrExitQualification->Fields.AccessType);
		break;
	}

}

/* Fill the guest's selector data */
/* 填充客户机的选择器数据 */
VOID HvFillGuestSelectorData(PVOID GdtBase, ULONG SegmentRegister, USHORT Selector)
{
	SEGMENT_SELECTOR SegmentSelector = { 0 };
	ULONG AccessRights;

	HvGetSegmentDescriptor(&SegmentSelector, Selector, GdtBase);
	AccessRights = ((PUCHAR)&SegmentSelector.ATTRIBUTES)[0] + (((PUCHAR)&SegmentSelector.ATTRIBUTES)[1] << 12);

	if (!Selector)
		AccessRights |= 0x10000;

	__vmx_vmwrite(GUEST_ES_SELECTOR + SegmentRegister * 2, Selector);
	__vmx_vmwrite(GUEST_ES_LIMIT + SegmentRegister * 2, SegmentSelector.LIMIT);
	__vmx_vmwrite(GUEST_ES_AR_BYTES + SegmentRegister * 2, AccessRights);
	__vmx_vmwrite(GUEST_ES_BASE + SegmentRegister * 2, SegmentSelector.BASE);

}

/* Handles in the cases when RDMSR causes a Vmexit*/
/* 处理RDMSR引起的VMexit情况 */
VOID HvHandleMsrRead(PGUEST_REGS GuestRegs)
{

	MSR msr = { 0 };


	// RDMSR. The RDMSR instruction causes a VM exit if any of the following are true:
	// 
	// The "use MSR bitmaps" VM-execution control is 0.
	// The value of ECX is not in the ranges 00000000H - 00001FFFH and C0000000H - C0001FFFH
	// The value of ECX is in the range 00000000H - 00001FFFH and bit n in read bitmap for low MSRs is 1,
	//   where n is the value of ECX.
	// The value of ECX is in the range C0000000H - C0001FFFH and bit n in read bitmap for high MSRs is 1,
	//   where n is the value of ECX & 00001FFFH.

    /* RDMSR指令。如果满足以下任一条件，RDMSR指令将导致VM退出：
	"使用MSR位图"的VM执行控制为0。
	ECX的值不在00000000H - 00001FFFH和C0000000H - C0001FFFH范围内。
	ECX的值在00000000H - 00001FFFH范围内，并且低MSRs的读取位图中的第n位为1，其中n为ECX的值。
	ECX的值在C0000000H - C0001FFFH范围内，并且高MSRs的读取位图中的第n位为1，其中n为ECX和00001FFFH的值。 */


	/*
	   Execute WRMSR or RDMSR on behalf of the guest. Important that this
	   can cause bug check when the guest tries to access unimplemented MSR
	   even within the SEH block* because the below WRMSR or RDMSR raises
	   #GP and are not protected by the SEH block (or cannot be protected
	   either as this code run outside the thread stack region Windows
	   requires to proceed SEH). Hypervisors typically handle this by noop-ing
	   WRMSR and returning zero for RDMSR with non-architecturally defined
	   MSRs. Alternatively, one can probe which MSRs should cause #GP prior
	   to installation of a hypervisor and the hypervisor can emulate the
	   results.
	   */
    /*
	代表客户机执行WRMSR或RDMSR指令。
	重要的是，当客户机尝试访问未实现的MSR时，即使在SEH块内部，这也可能导致蓝屏错误，
	因为下面的WRMSR或RDMSR会引发#GP，并且不受SEH块的保护（或者无法受保护，
	因为此代码运行在线程栈区域之外，而Windows需要在SEH中进行处理）。
	典型的hypervisor会通过对未定义MSR的WRMSR进行noop操作，并对非架构定义的MSR的RDMSR返回零来处理此问题。
	另外，也可以在安装hypervisor之前探测应该导致#GP的MSR，然后hypervisor可以模拟这些结果。
	*/

	// Check for sanity of MSR if they're valid or they're for reserved range for WRMSR and RDMSR
    // 检查MSR的合法性，判断它们是否有效或者是否属于WRMSR和RDMSR的保留范围
	if ((GuestRegs->rcx <= 0x00001FFF) || ((0xC0000000 <= GuestRegs->rcx) && (GuestRegs->rcx <= 0xC0001FFF))
		|| (GuestRegs->rcx >= RESERVED_MSR_RANGE_LOW && (GuestRegs->rcx <= RESERVED_MSR_RANGE_HI)))
	{
		msr.Content = __readmsr(GuestRegs->rcx);
	}

	GuestRegs->rax = msr.Low;
	GuestRegs->rdx = msr.High;
}

/* Handles in the cases when RDMSR causes a Vmexit*/
/* 处理RDMSR引起的VMexit情况 */
VOID HvHandleMsrWrite(PGUEST_REGS GuestRegs)
{
	MSR msr = { 0 };

	/*
	   Execute WRMSR or RDMSR on behalf of the guest. Important that this
	   can cause bug check when the guest tries to access unimplemented MSR
	   even within the SEH block* because the below WRMSR or RDMSR raises
	   #GP and are not protected by the SEH block (or cannot be protected
	   either as this code run outside the thread stack region Windows
	   requires to proceed SEH). Hypervisors typically handle this by noop-ing
	   WRMSR and returning zero for RDMSR with non-architecturally defined
	   MSRs. Alternatively, one can probe which MSRs should cause #GP prior
	   to installation of a hypervisor and the hypervisor can emulate the
	   results.
	   */
    /*
	代表客户机执行WRMSR或RDMSR指令。
	重要的是，当客户机尝试访问未实现的MSR时，即使在SEH块内部，
	这也可能导致蓝屏错误，因为下面的WRMSR或RDMSR会引发#GP，
	并且不受SEH块的保护（或者无法受保护，因为此代码运行在线程栈区域之外，而Windows需要在SEH中进行处理）。
	典型的hypervisor会通过对未定义MSR的WRMSR进行noop操作，并对非架构定义的MSR的RDMSR返回零来处理此问题。
	另外，也可以在安装hypervisor之前探测应该导致#GP的MSR，然后hypervisor可以模拟这些结果。
	*/

	// Check for sanity of MSR if they're valid or they're for reserved range for WRMSR and RDMSR
    // 检查MSR的有效性，判断它们是否合法或者属于WRMSR和RDMSR的保留范围
	if ((GuestRegs->rcx <= 0x00001FFF) || ((0xC0000000 <= GuestRegs->rcx) && (GuestRegs->rcx <= 0xC0001FFF))
		|| (GuestRegs->rcx >= RESERVED_MSR_RANGE_LOW && (GuestRegs->rcx <= RESERVED_MSR_RANGE_HI)))
	{
		msr.Low = (ULONG)GuestRegs->rax;
		msr.High = (ULONG)GuestRegs->rdx;
		__writemsr(GuestRegs->rcx, msr.Content);
	}

}

/* Set bits in Msr Bitmap */
/* 设置Msr位图中的位 */
BOOLEAN HvSetMsrBitmap(ULONG64 Msr, INT ProcessorID, BOOLEAN ReadDetection, BOOLEAN WriteDetection)
{

	if (!ReadDetection && !WriteDetection)
	{
		// Invalid Command
        // 无效的命令
		return FALSE;
	}

	if (Msr <= 0x00001FFF)
	{
		if (ReadDetection)
		{
			SetBit(GuestState[ProcessorID].MsrBitmapVirtualAddress, Msr, TRUE);
		}
		if (WriteDetection)
		{
			SetBit(GuestState[ProcessorID].MsrBitmapVirtualAddress + 2048, Msr, TRUE);
		}
	}
	else if ((0xC0000000 <= Msr) && (Msr <= 0xC0001FFF))
	{
		if (ReadDetection)
		{
			SetBit(GuestState[ProcessorID].MsrBitmapVirtualAddress + 1024, Msr - 0xC0000000, TRUE);
		}
		if (WriteDetection)
		{
			SetBit(GuestState[ProcessorID].MsrBitmapVirtualAddress + 3072, Msr - 0xC0000000, TRUE);

		}
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}

/* Add the current instruction length to guest rip to resume to next instruction */
/* 将当前指令长度添加到客户机的RIP，以便继续执行下一条指令 */
VOID HvResumeToNextInstruction()
{
	ULONG64 ResumeRIP = NULL;
	ULONG64 CurrentRIP = NULL;
	ULONG ExitInstructionLength = 0;

	__vmx_vmread(GUEST_RIP, &CurrentRIP);
	__vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstructionLength);

	ResumeRIP = CurrentRIP + ExitInstructionLength;

	__vmx_vmwrite(GUEST_RIP, ResumeRIP);
}

/* Notify all core to invalidate their EPT */
/* 通知所有核心使其无效化其EPT */
VOID HvNotifyAllToInvalidateEpt()
{
	// Let's notify them all
    // 让我们通知它们所有的核心
	KeIpiGenericCall(HvInvalidateEptByVmcall, EptState->EptPointer.Flags);
}

/* Invalidate EPT using Vmcall (should be called from Vmx non root mode) */
/* 使用Vmcall使EPT无效化（应该从Vmx非根模式调用） */
VOID HvInvalidateEptByVmcall(UINT64 Context)
{
	if (Context == NULL)
	{
		// We have to invalidate all contexts
        // 我们必须使所有上下文无效化
		AsmVmxVmcall(VMCALL_INVEPT_ALL_CONTEXTS, NULL, NULL, NULL);
	}
	else
	{
		// We have to invalidate all contexts
        // 我们必须使所有上下文无效化
		AsmVmxVmcall(VMCALL_INVEPT_SINGLE_CONTEXT, Context, NULL, NULL);
	}
}


/* Returns the stack pointer, to change in the case of Vmxoff */
/* 返回堆栈指针，在Vmxoff的情况下进行更改 */
UINT64 HvReturnStackPointerForVmxoff()
{
	return GuestState[KeGetCurrentProcessorNumber()].VmxoffState.GuestRsp;
}

/* Returns the instruction pointer, to change in the case of Vmxoff */
/* 返回指令指针，在Vmxoff的情况下进行更改 */
UINT64 HvReturnInstructionPointerForVmxoff()
{
	return GuestState[KeGetCurrentProcessorNumber()].VmxoffState.GuestRip;
}


/* The broadcast function which initialize the guest. */
/* 初始化客户机的广播函数 */
VOID HvDpcBroadcastInitializeGuest(KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
	// Save the vmx state and prepare vmcs setup and finally execute vmlaunch instruction
    // 保存vmx状态并准备vmcs设置，最后执行vmlaunch指令
	AsmVmxSaveState();

	// Wait for all DPCs to synchronize at this point
    // 在此处等待所有的延迟处理程序（DPC）同步
	KeSignalCallDpcSynchronize(SystemArgument2);

	// Mark the DPC as being complete
    // 将延迟处理程序（DPC）标记为已完成
	KeSignalCallDpcDone(SystemArgument1);
}

/* Terminate Vmx on all logical cores. */
/* 在所有逻辑核心上终止Vmx。 */
VOID HvTerminateVmx()
{
	// Remve All the hooks if any
    // 如果有任何钩子，移除所有钩子。
	HvPerformPageUnHookAllPages();

	// Broadcast to terminate Vmx
    // 广播终止Vmx命令。
	KeGenericCallDpc(HvDpcBroadcastTerminateGuest, 0x0);
	
	/* De-allocatee global variables */
    /* 释放全局变量的内存空间 */

	// Free Identity Page Table
    // 释放身份页表（Identity Page Table）。
	MmFreeContiguousMemory(EptState->EptPageTable);

	// Free EptState
    // 释放EptState（扩展页表状态）。
	ExFreePoolWithTag(EptState, POOLTAG);

	// Free the Pool manager
    // 释放池管理器（Pool manager）。
	PoolManagerUninitialize();
	
}

/* The broadcast function which terminate the guest. */
/* 终止客户端的广播函数。 */
VOID HvDpcBroadcastTerminateGuest(KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{	
	// Terminate Vmx using Vmcall
    // 使用Vmcall终止Vmx。
	if (!VmxTerminate())
	{
		// Not serving IOCTL Here, so use DbgPrint
        // 在此处不提供IOCTL服务，因此使用DbgPrint。
		DbgPrint("There were an error terminating Vmx");
		DbgBreakPoint();
	}
	
	// Wait for all DPCs to synchronize at this point
    // 在此处等待所有的延迟处理程序（DPC）同步。
	KeSignalCallDpcSynchronize(SystemArgument2);

	// Mark the DPC as being complete
    // 将延迟处理程序（DPC）标记为已完成。
	KeSignalCallDpcDone(SystemArgument1);
}

/* Set the monitor trap flag */
/* 设置监视陷阱标志（monitor trap flag） */
VOID HvSetMonitorTrapFlag(BOOLEAN Set)
{
	ULONG CpuBasedVmExecControls = 0;

	// Read the previous flag
    // 读取先前的标志位（flag）
	__vmx_vmread(CPU_BASED_VM_EXEC_CONTROL, &CpuBasedVmExecControls);

	if (Set) {
		CpuBasedVmExecControls |= CPU_BASED_MONITOR_TRAP_FLAG;
	}
	else {
		CpuBasedVmExecControls &= ~CPU_BASED_MONITOR_TRAP_FLAG;
	}

	// Set the new value 
	// 设置新的值
	__vmx_vmwrite(CPU_BASED_VM_EXEC_CONTROL, CpuBasedVmExecControls);
}

/* Reset GDTR/IDTR and other old when you do vmxoff as the patchguard will detect them left modified */
/* 当执行vmxoff时，重置GDTR/IDTR和其他旧值，因为PatchGuard会检测到它们被修改了。 */
VOID HvRestoreRegisters()
{
	ULONG64 FsBase;
	ULONG64 GsBase;
	ULONG64 GdtrBase;
	ULONG64 GdtrLimit;
	ULONG64 IdtrBase;
	ULONG64 IdtrLimit;

	// Restore FS Base 
	// 恢复FS基址（FS Base）。
	__vmx_vmread(GUEST_FS_BASE, &FsBase);
	__writemsr(MSR_FS_BASE, FsBase);

	// Restore Gs Base
    // 恢复GS基址（GS Base）。
	__vmx_vmread(GUEST_GS_BASE, &GsBase);
	__writemsr(MSR_GS_BASE, GsBase);

	// Restore GDTR
    // 恢复GDTR（全局描述符表寄存器）。
	__vmx_vmread(GUEST_GDTR_BASE, &GdtrBase);
	__vmx_vmread(GUEST_GDTR_LIMIT, &GdtrLimit);

	AsmReloadGdtr(GdtrBase, GdtrLimit);

	// Restore IDTR
    // 恢复IDTR（中断描述符表寄存器）。
	__vmx_vmread(GUEST_IDTR_BASE, &IdtrBase);
	__vmx_vmread(GUEST_IDTR_LIMIT, &IdtrLimit);

	AsmReloadIdtr(IdtrBase, IdtrLimit);
}

/* The broadcast function which removes all the hooks and invalidate TLB. */
/* 移除所有的钩子并使TLB失效的广播函数。 */
VOID HvDpcBroadcastRemoveHookAndInvalidateAllEntries(KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
	// Execute the VMCALL to remove the hook and invalidate
    // 执行VMCALL以移除钩子并使其失效。
	AsmVmxVmcall(VMCALL_UNHOOK_ALL_PAGES, NULL, NULL, NULL);

	// Wait for all DPCs to synchronize at this point
    // 在此处等待所有的延迟处理程序（DPC）同步。
	KeSignalCallDpcSynchronize(SystemArgument2);

	// Mark the DPC as being complete
    // 将延迟处理程序（DPC）标记为已完成。
	KeSignalCallDpcDone(SystemArgument1);
}

/* The broadcast function which removes the single hook and invalidate TLB. */
/* 移除单个钩子并使TLB失效的广播函数。 */
VOID HvDpcBroadcastRemoveHookAndInvalidateSingleEntry(KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
	// Execute the VMCALL to remove the hook and invalidate
    // 执行VMCALL以移除钩子并使其失效。
	AsmVmxVmcall(VMCALL_UNHOOK_SINGLE_PAGE, DeferredContext, NULL, NULL);

	// Wait for all DPCs to synchronize at this point
    // 在此处等待所有的延迟处理程序（DPC）同步。
	KeSignalCallDpcSynchronize(SystemArgument2);

	// Mark the DPC as being complete
    // 将延迟处理程序（DPC）标记为已完成。
	KeSignalCallDpcDone(SystemArgument1);
}


/* Remove single hook from the hooked pages list and invalidate TLB */
/* 从钩住页面列表中移除单个钩子并使TLB失效 */
BOOLEAN HvPerformPageUnHookSinglePage(UINT64 VirtualAddress) {
	PLIST_ENTRY TempList = 0;
	SIZE_T PhysicalAddress;

	PhysicalAddress = PAGE_ALIGN(VirtualAddressToPhysicalAddress(VirtualAddress));

	// Should be called from vmx non-root
    // 应该从vmx非根模式下调用。
	if (GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
	{
		return FALSE;
	}

	TempList = &EptState->HookedPagesList;
	while (&EptState->HookedPagesList != TempList->Flink)
	{
		TempList = TempList->Flink;
		PEPT_HOOKED_PAGE_DETAIL HookedEntry = CONTAINING_RECORD(TempList, EPT_HOOKED_PAGE_DETAIL, PageHookList);

		if (HookedEntry->PhysicalBaseAddress == PhysicalAddress)
		{
			// Remove it in all the cores
            // 在所有核心中移除它。
			KeGenericCallDpc(HvDpcBroadcastRemoveHookAndInvalidateSingleEntry, HookedEntry->PhysicalBaseAddress);

			// remove the entry from the list
            // 从列表中移除该条目。
			RemoveEntryList(HookedEntry->PageHookList.Flink);

			return TRUE;
		}
	}
	// Nothing found , probably the list is not found
    // 未找到任何内容，可能列表不存在。
	return FALSE;
}

/* Remove all hooks from the hooked pages list and invalidate TLB */
// Should be called from Vmx Non-root
/* 从钩住页面列表中移除所有钩子并使TLB失效 */
// 应该从Vmx非根模式下调用。
VOID HvPerformPageUnHookAllPages() {

	// Should be called from vmx non-root
    // 应该从vmx非根模式下调用。
	if (GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
	{
		return;
	}

	// Remove it in all the cores
    // 在所有核心中移除它。
	KeGenericCallDpc(HvDpcBroadcastRemoveHookAndInvalidateAllEntries, 0x0);

	// No need to remove the list as it will automatically remove by the pool uninitializer
    // 不需要手动移除列表，因为它将在池反初始化时自动移除。
}
