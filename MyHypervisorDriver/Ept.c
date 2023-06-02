#include "Vmx.h"
#include "Ept.h"
#include "Common.h"
#include "InlineAsm.h"
#include "GlobalVariables.h"
#include "Invept.h"
#include "HypervisorRoutines.h"
#include "Vmcall.h"
#include "PoolManager.h"

/* Check whether EPT features are present or not */
/* 检查 EPT 功能是否存在 */
BOOLEAN EptCheckFeatures()
{
	IA32_VMX_EPT_VPID_CAP_REGISTER VpidRegister;
	IA32_MTRR_DEF_TYPE_REGISTER MTRRDefType;

	VpidRegister.Flags = __readmsr(MSR_IA32_VMX_EPT_VPID_CAP);
	MTRRDefType.Flags = __readmsr(MSR_IA32_MTRR_DEF_TYPE);

	if (!VpidRegister.PageWalkLength4 || !VpidRegister.MemoryTypeWriteBack || !VpidRegister.Pde2MbPages)
	{
		return FALSE;
	}

	if (!VpidRegister.AdvancedVmexitEptViolationsInformation)
	{
		LogWarning("The processor doesn't report advanced VM-exit information for EPT violations");
	}

	if (!VpidRegister.ExecuteOnlyPages)
	{
		ExecuteOnlySupport = FALSE;
		LogWarning("The processor doesn't support execute-only pages, execute hooks won't work as they're on this feature in our design");
	}
	else
	{
		ExecuteOnlySupport = TRUE;
	}

	if (!MTRRDefType.MtrrEnable)
	{
		LogError("Mtrr Dynamic Ranges not supported");
		return FALSE;
	}

	LogInfo(" *** All EPT features are present *** ");

	return TRUE;
}


/* Build MTRR Map of current physical addresses */
/* 构建当前物理地址的 MTRR 映射 */
BOOLEAN EptBuildMtrrMap()
{
	IA32_MTRR_CAPABILITIES_REGISTER MTRRCap;
	IA32_MTRR_PHYSBASE_REGISTER CurrentPhysBase;
	IA32_MTRR_PHYSMASK_REGISTER CurrentPhysMask;
	PMTRR_RANGE_DESCRIPTOR Descriptor;
	ULONG CurrentRegister;
	ULONG NumberOfBitsInMask;

	MTRRCap.Flags = __readmsr(MSR_IA32_MTRR_CAPABILITIES);

	for (CurrentRegister = 0; CurrentRegister < MTRRCap.VariableRangeCount; CurrentRegister++)
	{
		// For each dynamic register pair
        // 循环每个动态寄存器
		CurrentPhysBase.Flags = __readmsr(MSR_IA32_MTRR_PHYSBASE0 + (CurrentRegister * 2));
		CurrentPhysMask.Flags = __readmsr(MSR_IA32_MTRR_PHYSMASK0 + (CurrentRegister * 2));

		// Is the range enabled?
        // 是否启用了该范围？
		if (CurrentPhysMask.Valid)
		{
			// We only need to read these once because the ISA dictates that MTRRs are to be synchronized between all processors
			// during BIOS initialization.
            // 我们只需要在初始化期间读取一次这些值，因为ISA规定在BIOS初始化期间MTRR应在所有处理器之间同步。
			Descriptor = &EptState->MemoryRanges[EptState->NumberOfEnabledMemoryRanges++];

			// Calculate the base address in bytes
            // 计算基地址的字节数。
			Descriptor->PhysicalBaseAddress = CurrentPhysBase.PageFrameNumber * PAGE_SIZE;

			// Calculate the total size of the range
			// The lowest bit of the mask that is set to 1 specifies the size of the range
            // 计算范围的总大小。
            // 设置为1的掩码的最低位指定范围的大小。
			_BitScanForward64(&NumberOfBitsInMask, CurrentPhysMask.PageFrameNumber * PAGE_SIZE);

			// Size of the range in bytes + Base Address
            // 范围的大小（以字节为单位）加上基地址。
			Descriptor->PhysicalEndAddress = Descriptor->PhysicalBaseAddress + ((1ULL << NumberOfBitsInMask) - 1ULL);

			// Memory Type (cacheability attributes)
            // 内存类型（缓存属性）
			Descriptor->MemoryType = (UCHAR)CurrentPhysBase.Type;

			if (Descriptor->MemoryType == MEMORY_TYPE_WRITE_BACK)
			{
				/* This is already our default, so no need to store this range.
				 * Simply 'free' the range we just wrote. */
                /* 这已经是我们的默认设置，所以不需要存储这个范围。
				只需“释放”我们刚刚写入的范围即可。 */
				EptState->NumberOfEnabledMemoryRanges--;
			}
			LogInfo("MTRR Range: Base=0x%llx End=0x%llx Type=0x%x", Descriptor->PhysicalBaseAddress, Descriptor->PhysicalEndAddress, Descriptor->MemoryType);
		}
	}

	LogInfo("Total MTRR Ranges Committed: %d", EptState->NumberOfEnabledMemoryRanges);

	return TRUE;
}

/* Get the PML1 entry for this physical address if the page is split. Return NULL if the address is invalid or the page wasn't already split. */
/* 如果页面已经被拆分，则获取此物理地址的PML1条目。如果地址无效或页面尚未被拆分，则返回NULL。 */
PEPT_PML1_ENTRY EptGetPml1Entry(PVMM_EPT_PAGE_TABLE EptPageTable, SIZE_T PhysicalAddress)
{
	SIZE_T Directory, DirectoryPointer, PML4Entry;
	PEPT_PML2_ENTRY PML2;
	PEPT_PML1_ENTRY PML1;
	PEPT_PML2_POINTER PML2Pointer;

	Directory = ADDRMASK_EPT_PML2_INDEX(PhysicalAddress);
	DirectoryPointer = ADDRMASK_EPT_PML3_INDEX(PhysicalAddress);
	PML4Entry = ADDRMASK_EPT_PML4_INDEX(PhysicalAddress);

	// Addresses above 512GB are invalid because it is > physical address bus width 
	// 地址超过512GB是无效的，因为它超过了物理地址总线的宽度。
	if (PML4Entry > 0)
	{
		return NULL;
	}

	PML2 = &EptPageTable->PML2[DirectoryPointer][Directory];

	// Check to ensure the page is split 
	// 检查以确保页面已经分割。
	if (PML2->LargePage)
	{
		return NULL;
	}

	// Conversion to get the right PageFrameNumber.
	// These pointers occupy the same place in the table and are directly convertable.
    // 进行转换以获得正确的 PageFrameNumber。
    // 这些指针在表中占据相同的位置，可以直接转换。
	PML2Pointer = (PEPT_PML2_POINTER)PML2;

	// If it is, translate to the PML1 pointer 
	// 如果是的话，将其转换为 PML1 指针
	PML1 = (PEPT_PML1_ENTRY)PhysicalAddressToVirtualAddress((PVOID)(PML2Pointer->PageFrameNumber * PAGE_SIZE));

	if (!PML1)
	{
		return NULL;
	}

	// Index into PML1 for that address 
	// 根据该地址索引到 PML1 的位置
	PML1 = &PML1[ADDRMASK_EPT_PML1_INDEX(PhysicalAddress)];

	return PML1;
}


/* Get the PML2 entry for this physical address. */
// 根据该物理地址获取 PML2 的条目。
PEPT_PML2_ENTRY EptGetPml2Entry(PVMM_EPT_PAGE_TABLE EptPageTable, SIZE_T PhysicalAddress)
{
	SIZE_T Directory, DirectoryPointer, PML4Entry;
	PEPT_PML2_ENTRY PML2;

	Directory = ADDRMASK_EPT_PML2_INDEX(PhysicalAddress);
	DirectoryPointer = ADDRMASK_EPT_PML3_INDEX(PhysicalAddress);
	PML4Entry = ADDRMASK_EPT_PML4_INDEX(PhysicalAddress);

	// Addresses above 512GB are invalid because it is > physical address bus width 
	// 大于512GB的地址是无效的，因为它超过了物理地址总线的宽度。
	if (PML4Entry > 0)
	{
		return NULL;
	}

	PML2 = &EptPageTable->PML2[DirectoryPointer][Directory];
	return PML2;
}

/* Split 2MB (LargePage) into 4kb pages */
/* 将2MB（大页）拆分为4KB页面 */
BOOLEAN EptSplitLargePage(PVMM_EPT_PAGE_TABLE EptPageTable, PVOID PreAllocatedBuffer, SIZE_T PhysicalAddress, ULONG CoreIndex)
{

	PVMM_EPT_DYNAMIC_SPLIT NewSplit;
	EPT_PML1_ENTRY EntryTemplate;
	SIZE_T EntryIndex;
	PEPT_PML2_ENTRY TargetEntry;
	EPT_PML2_POINTER NewPointer;

	// Find the PML2 entry that's currently used
    // 找到当前正在使用的 PML2 项
	TargetEntry = EptGetPml2Entry(EptPageTable, PhysicalAddress);
	if (!TargetEntry)
	{
		LogError("An invalid physical address passed");
		return FALSE;
	}

	// If this large page is not marked a large page, that means it's a pointer already.
	// That page is therefore already split.
    // 如果这个大页面没有标记为大页面，那意味着它已经是一个指针。
    // 这个页面已经被分割了。
	if (!TargetEntry->LargePage)
	{
		return TRUE;
	}

	// Allocate the PML1 entries 
	// 分配PML1条目
	NewSplit = (PVMM_EPT_DYNAMIC_SPLIT)PreAllocatedBuffer;
	if (!NewSplit)
	{
		LogError("Failed to allocate dynamic split memory");
		return FALSE;
	}
	RtlZeroMemory(NewSplit, sizeof(VMM_EPT_DYNAMIC_SPLIT));


	// Point back to the entry in the dynamic split for easy reference for which entry that dynamic split is for.
    // 将指针指向与动态分割对应的条目，以便在需要时可以轻松引用。
	NewSplit->Entry = TargetEntry;

	// Make a template for RWX 
	// 创建一个可读、可写、可执行 (RWX) 的模板。
	EntryTemplate.Flags = 0;
	EntryTemplate.ReadAccess = 1;
	EntryTemplate.WriteAccess = 1;
	EntryTemplate.ExecuteAccess = 1;

	// Copy the template into all the PML1 entries 
	// 将模板复制到所有的 PML1 条目中。
	__stosq((SIZE_T*)&NewSplit->PML1[0], EntryTemplate.Flags, VMM_EPT_PML1E_COUNT);


	// Set the page frame numbers for identity mapping.
    // 设置页面框号以进行标识映射。
	for (EntryIndex = 0; EntryIndex < VMM_EPT_PML1E_COUNT; EntryIndex++)
	{
		// Convert the 2MB page frame number to the 4096 page entry number plus the offset into the frame. 
		// 将2MB页面的页面框号转换为4096页面条目号加上页面框内的偏移量。
		NewSplit->PML1[EntryIndex].PageFrameNumber = ((TargetEntry->PageFrameNumber * SIZE_2_MB) / PAGE_SIZE) + EntryIndex;
	}

	// Allocate a new pointer which will replace the 2MB entry with a pointer to 512 4096 byte entries. 
	// 分配一个新的指针，它将替换2MB条目，并指向512个4096字节的条目。
	NewPointer.Flags = 0;
	NewPointer.WriteAccess = 1;
	NewPointer.ReadAccess = 1;
	NewPointer.ExecuteAccess = 1;
	NewPointer.PageFrameNumber = (SIZE_T)VirtualAddressToPhysicalAddress(&NewSplit->PML1[0]) / PAGE_SIZE;


	// Now, replace the entry in the page table with our new split pointer.
    // 现在，用我们的新分割指针替换页表中的条目。
	RtlCopyMemory(TargetEntry, &NewPointer, sizeof(NewPointer));

	return TRUE;
}



/* Set up PML2 Entries */
/* 设置 PML2 条目 */
VOID EptSetupPML2Entry(PEPT_PML2_ENTRY NewEntry, SIZE_T PageFrameNumber)
{
	SIZE_T AddressOfPage;
	SIZE_T CurrentMtrrRange;
	SIZE_T TargetMemoryType;

	/*
	  Each of the 512 collections of 512 PML2 entries is setup here.
	  This will, in total, identity map every physical address from 0x0 to physical address 0x8000000000 (512GB of memory)

	  ((EntryGroupIndex * VMM_EPT_PML2E_COUNT) + EntryIndex) * 2MB is the actual physical address we're mapping
	 */
    /*
	这里设置了每个 512 个 PML2 条目的集合。
	这将总共从 0x0 到物理地址 0x8000000000（512GB 内存）进行身份映射

	((EntryGroupIndex * VMM_EPT_PML2E_COUNT) + EntryIndex) * 2MB 是我们正在映射的实际物理地址
	*/
	NewEntry->PageFrameNumber = PageFrameNumber;

	// Size of 2MB page * PageFrameNumber == AddressOfPage (physical memory). 
	// 2MB 页面大小 * PageFrameNumber == AddressOfPage（物理内存地址）。
	AddressOfPage = PageFrameNumber * SIZE_2_MB;

	/* To be safe, we will map the first page as UC as to not bring up any kind of undefined behavior from the
	  fixed MTRR section which we are not formally recognizing (typically there is MMIO memory in the first MB).

	  I suggest reading up on the fixed MTRR section of the manual to see why the first entry is likely going to need to be UC.
	 */
    /* 为了安全起见，我们将将第一个页面映射为UC，以避免从我们没有正式识别的固定MTRR部分引起任何未定义行为（通常在前1MB中存在MMIO内存）。
	我建议阅读手册上关于固定MTRR部分的内容，了解为什么第一个条目可能需要设置为UC。 
	*/
	if (PageFrameNumber == 0)
	{
		NewEntry->MemoryType = MEMORY_TYPE_UNCACHEABLE;
		return;
	}

	// Default memory type is always WB for performance. 
	// 默认的内存类型始终是WB（Write-Back），以提供更好的性能。
	TargetMemoryType = MEMORY_TYPE_WRITE_BACK;

	// For each MTRR range 
	// 循环每个MTRR范围
	for (CurrentMtrrRange = 0; CurrentMtrrRange < EptState->NumberOfEnabledMemoryRanges; CurrentMtrrRange++)
	{
		// If this page's address is below or equal to the max physical address of the range 
		// 如果此页的地址小于或等于范围的最大物理地址
		if (AddressOfPage <= EptState->MemoryRanges[CurrentMtrrRange].PhysicalEndAddress)
		{
			// And this page's last address is above or equal to the base physical address of the range 
			// 并且此页的最后地址大于或等于范围的基本物理地址
			if ((AddressOfPage + SIZE_2_MB - 1) >= EptState->MemoryRanges[CurrentMtrrRange].PhysicalBaseAddress)
			{
				/* If we're here, this page fell within one of the ranges specified by the variable MTRRs
				   Therefore, we must mark this page as the same cache type exposed by the MTRR
				 */
                // 如果我们到达这里，说明此页位于由变量MTRRs指定的范围内
                // 因此，我们必须将此页标记为与MTRR公开的相同缓存类型
				TargetMemoryType = EptState->MemoryRanges[CurrentMtrrRange].MemoryType;
				// LogInfo("0x%X> Range=%llX -> %llX | Begin=%llX End=%llX", PageFrameNumber, AddressOfPage, AddressOfPage + SIZE_2_MB - 1, EptState->MemoryRanges[CurrentMtrrRange].PhysicalBaseAddress, EptState->MemoryRanges[CurrentMtrrRange].PhysicalEndAddress);

				// 11.11.4.1 MTRR Precedences 
				// 11.11.4.1 MTRR 优先级
                /*
				根据Intel手册的规定，MTRR的优先级如下：
				UC（Uncacheable）：最高优先级，表示该范围的内存是不可缓存的。
				WC（Write-Combining）：次高优先级，表示该范围的内存用于写合并操作。
				WT（Write-Through）：中等优先级，表示该范围的内存使用写透传方式。
				WP（Write-Protected）：较低优先级，表示该范围的内存是只读的。
				WB（Write-Back）：最低优先级，表示该范围的内存使用写回方式。
				*/
				if (TargetMemoryType == MEMORY_TYPE_UNCACHEABLE)
				{
					// If this is going to be marked uncacheable, then we stop the search as UC always takes precedent.
                    // 如果将其标记为不可缓存（UC），则我们停止搜索，因为UC始终具有优先权。
					break;
				}
			}
		}
	}

	// Finally, commit the memory type to the entry. 
	// 最后，将内存类型设置应用到页表项中。
	NewEntry->MemoryType = TargetMemoryType;
}

/* Allocates page maps and create identity page table */
/* 分配页映射并创建身份页表 */
PVMM_EPT_PAGE_TABLE EptAllocateAndCreateIdentityPageTable()
{
	PVMM_EPT_PAGE_TABLE PageTable;
	EPT_PML3_POINTER RWXTemplate;
	EPT_PML2_ENTRY PML2EntryTemplate;
	SIZE_T EntryGroupIndex;
	SIZE_T EntryIndex;

	// Allocate all paging structures as 4KB aligned pages 
	// 将所有分页结构分配为4KB对齐的页
	PHYSICAL_ADDRESS MaxSize;
	PVOID Output;

	// Allocate address anywhere in the OS's memory space
    // 在操作系统的内存空间中分配地址的任意位置
	MaxSize.QuadPart = MAXULONG64;

	PageTable = MmAllocateContiguousMemory((sizeof(VMM_EPT_PAGE_TABLE) / PAGE_SIZE) * PAGE_SIZE, MaxSize);

	if (PageTable == NULL)
	{
		LogError("Failed to allocate memory for PageTable");
		return NULL;
	}

	// Zero out all entries to ensure all unused entries are marked Not Present 
	// 将所有条目清零，确保所有未使用的条目都标记为未存在
	RtlZeroMemory(PageTable, sizeof(VMM_EPT_PAGE_TABLE));

	// Mark the first 512GB PML4 entry as present, which allows us to manage up to 512GB of discrete paging structures.
    // 将第一个512GB的PML4条目标记为存在，这样我们就可以管理高达512GB的离散分页结构。
	PageTable->PML4[0].PageFrameNumber = (SIZE_T)VirtualAddressToPhysicalAddress(&PageTable->PML3[0]) / PAGE_SIZE;
	PageTable->PML4[0].ReadAccess = 1;
	PageTable->PML4[0].WriteAccess = 1;
	PageTable->PML4[0].ExecuteAccess = 1;

	/* Now mark each 1GB PML3 entry as RWX and map each to their PML2 entry */
    // 现在将每个1GB的PML3条目标记为RWX，并将每个条目映射到其对应的PML2条目。

	// Ensure stack memory is cleared
    // 确保堆栈内存已清空。
	RWXTemplate.Flags = 0;

	// Set up one 'template' RWX PML3 entry and copy it into each of the 512 PML3 entries 
	// Using the same method as SimpleVisor for copying each entry using intrinsics. 
	// 设置一个“模板”RWX PML3条目，并将其复制到每个512个PML3条目中
    // 使用与SimpleVisor相同的方法，使用内部函数复制每个条目。
	RWXTemplate.ReadAccess = 1;
	RWXTemplate.WriteAccess = 1;
	RWXTemplate.ExecuteAccess = 1;

	// Copy the template into each of the 512 PML3 entry slots 
	// 将模板复制到每个512个PML3条目的位置上
	__stosq((SIZE_T*)&PageTable->PML3[0], RWXTemplate.Flags, VMM_EPT_PML3E_COUNT);

	// For each of the 512 PML3 entries 
	// 循环每个512个PML3条目
	for (EntryIndex = 0; EntryIndex < VMM_EPT_PML3E_COUNT; EntryIndex++)
	{
		// Map the 1GB PML3 entry to 512 PML2 (2MB) entries to describe each large page.
		// NOTE: We do *not* manage any PML1 (4096 byte) entries and do not allocate them.
        // 将1GB的PML3条目映射到512个PML2（2MB）条目，以描述每个大页。
        // 注意：我们不管理任何PML1（4096字节）条目，也不分配它们。
		PageTable->PML3[EntryIndex].PageFrameNumber = (SIZE_T)VirtualAddressToPhysicalAddress(&PageTable->PML2[EntryIndex][0]) / PAGE_SIZE;
	}

	PML2EntryTemplate.Flags = 0;

	// All PML2 entries will be RWX and 'present' 
	// 所有PML2条目将是RWX（可读、可写、可执行）且为“存在”状态。
	PML2EntryTemplate.WriteAccess = 1;
	PML2EntryTemplate.ReadAccess = 1;
	PML2EntryTemplate.ExecuteAccess = 1;

	// We are using 2MB large pages, so we must mark this 1 here. 
	// 我们使用的是2MB的大页，所以在这里我们必须将其标记为1。
	PML2EntryTemplate.LargePage = 1;

	/* For each collection of 512 PML2 entries (512 collections * 512 entries per collection), mark it RWX using the same template above.
	   This marks the entries as "Present" regardless of if the actual system has memory at this region or not. We will cause a fault in our
	   EPT handler if the guest access a page outside a usable range, despite the EPT frame being present here.
	 */
    /* 循环每个包含512个PML2条目的集合（512个集合 * 每个集合的512个条目），使用上面相同的模板将其标记为RWX。
	这将标记条目为“存在”，无论实际系统是否在此区域具有内存。如果客户机访问可用范围之外的页面，尽管EPT帧在此处存在，我们将在EPT处理程序中引发故障。
	*/
	__stosq((SIZE_T*)&PageTable->PML2[0], PML2EntryTemplate.Flags, VMM_EPT_PML3E_COUNT * VMM_EPT_PML2E_COUNT);

	// For each of the 512 collections of 512 2MB PML2 entries 
	// 循环每个512个512个2MB PML2条目的集合
	for (EntryGroupIndex = 0; EntryGroupIndex < VMM_EPT_PML3E_COUNT; EntryGroupIndex++)
	{
		// For each 2MB PML2 entry in the collection 
		// 循环集合中的每个2MB PML2条目
		for (EntryIndex = 0; EntryIndex < VMM_EPT_PML2E_COUNT; EntryIndex++)
		{
			// Setup the memory type and frame number of the PML2 entry. 
			// 设置PML2条目的内存类型和帧号码。
			EptSetupPML2Entry(&PageTable->PML2[EntryGroupIndex][EntryIndex], (EntryGroupIndex * VMM_EPT_PML2E_COUNT) + EntryIndex);
		}
	}

	return PageTable;
}


/*
  Initialize EPT for an individual logical processor.
  Creates an identity mapped page table and sets up an EPTP to be applied to the VMCS later.
*/
/*
初始化一个逻辑处理器的EPT。
创建一个标识映射的页表，并设置一个EPTP以供之后应用于VMCS。
*/
BOOLEAN EptLogicalProcessorInitialize()
{
	PVMM_EPT_PAGE_TABLE PageTable;
	EPTP EPTP;

	/* Allocate the identity mapped page table*/
    /* 分配标识映射的页表 */
	PageTable = EptAllocateAndCreateIdentityPageTable();
	if (!PageTable)
	{
		LogError("Unable to allocate memory for EPT");
		return FALSE;
	}

	// Virtual address to the page table to keep track of it for later freeing 
	// 用于后续释放的页表的虚拟地址
	EptState->EptPageTable = PageTable;

	EPTP.Flags = 0;

	// For performance, we let the processor know it can cache the EPT.
    // 为了性能，我们让处理器知道它可以缓存EPT。
	EPTP.MemoryType = MEMORY_TYPE_WRITE_BACK;

	// We are not utilizing the 'access' and 'dirty' flag features. 
	// 我们不使用'access'和'dirty'标志位功能。
	EPTP.EnableAccessAndDirtyFlags = FALSE;

	/*
	  Bits 5:3 (1 less than the EPT page-walk length) must be 3, indicating an EPT page-walk length of 4;
	  see Section 28.2.2
	 */
    /*
	Bits 5:3（EPT页面步长减1）必须为3，表示EPT页面步长为4；
	参见第28.2.2节
	*/
	EPTP.PageWalkLength = 3;

	// The physical page number of the page table we will be using 
	// 我们将使用的页表的物理页号
	EPTP.PageFrameNumber = (SIZE_T)VirtualAddressToPhysicalAddress(&PageTable->PML4) / PAGE_SIZE;

	// We will write the EPTP to the VMCS later 
	// 我们将在之后将EPTP写入VMCS
	EptState->EptPointer = EPTP;

	return TRUE;
}


/* Check if this exit is due to a violation caused by a currently hooked page. Returns FALSE
 * if the violation was not due to a page hook.
 *
 * If the memory access attempt was RW and the page was marked executable, the page is swapped with
 * the original page.
 *
 * If the memory access attempt was execute and the page was marked not executable, the page is swapped with
 * the hooked page.
 */
/* 检查这个退出是否是由于当前挂钩页的违规引起的。如果违规不是由于页面挂钩引起，返回FALSE。

如果内存访问尝试是RW并且页面标记为可执行，将页面与原始页面交换。

如果内存访问尝试是执行并且页面标记为不可执行，将页面与挂钩页面交换。
*/
BOOLEAN EptHandlePageHookExit(VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification, UINT64 GuestPhysicalAddr)
{
	BOOLEAN IsHandled = FALSE;
	PLIST_ENTRY TempList = 0;

	TempList = &EptState->HookedPagesList;
	while (&EptState->HookedPagesList != TempList->Flink)
	{
		TempList = TempList->Flink;
		PEPT_HOOKED_PAGE_DETAIL HookedEntry = CONTAINING_RECORD(TempList, EPT_HOOKED_PAGE_DETAIL, PageHookList);
		if (HookedEntry->PhysicalBaseAddress == PAGE_ALIGN(GuestPhysicalAddr))
		{
			/* We found an address that match the details */

			/*
			   Returning true means that the caller should return to the ept state to the previous state when this instruction is executed
			   by setting the Monitor Trap Flag. Return false means that nothing special for the caller to do
			*/

			/* 我们找到了一个与详细信息匹配的地址

			返回true意味着调用者应该通过设置Monitor Trap Flag将ept状态返回到之前的状态，当此指令被执行时。返回false意味着调用者无需做任何特殊处理。
			*/
			if (EptHandleHookedPage(HookedEntry, ViolationQualification, GuestPhysicalAddr))
			{
				// Next we have to save the current hooked entry to restore on the next instruction's vm-exit
                // 接下来，我们需要保存当前的挂钩条目，以便在下一条指令的vm-exit时恢复它
				GuestState[KeGetCurrentProcessorNumber()].MtfEptHookRestorePoint = HookedEntry;

				// We have to set Monitor trap flag and give it the HookedEntry to work with
                // 我们需要设置Monitor Trap Flag，并提供给它要使用的HookedEntry。
				HvSetMonitorTrapFlag(TRUE);


			}

			// Indicate that we handled the ept violation
            // 表示我们处理了ept违规事件
			IsHandled = TRUE;

			// Get out of the loop
            // 退出循环
			break;
		}
	}
	// Redo the instruction 
	// 重新执行指令
	GuestState[KeGetCurrentProcessorNumber()].IncrementRip = FALSE;
	return IsHandled;

}


/*
   Handle VM exits for EPT violations. Violations are thrown whenever an operation is performed
   on an EPT entry that does not provide permissions to access that page.
 */
/*
处理EPT违规的VM退出。违规事件在对不具备访问该页面权限的EPT条目执行操作时引发。
*/
BOOLEAN EptHandleEptViolation(ULONG ExitQualification, UINT64 GuestPhysicalAddr)
{

	VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification;

	ViolationQualification.Flags = ExitQualification;

	if (EptHandlePageHookExit(ViolationQualification, GuestPhysicalAddr))
	{
		// Handled by page hook code.
        // 由页面挂钩代码处理。
		return TRUE;
	}


	LogError("Unexpected EPT violation");

	// Redo the instruction that caused the exception. 
	// 重新执行导致异常的指令。
	return FALSE;
}

/* Handle vm-exits for Monitor Trap Flag to restore previous state */
/* 处理Monitor Trap Flag导致的vm-exit，以恢复先前的状态 */
VOID EptHandleMonitorTrapFlag(PEPT_HOOKED_PAGE_DETAIL HookedEntry)
{
	// restore the hooked state
    // 恢复挂钩状态
	EptSetPML1AndInvalidateTLB(HookedEntry->EntryAddress, HookedEntry->ChangedEntry, INVEPT_SINGLE_CONTEXT);

}
VOID EptHandleMisconfiguration(UINT64 GuestAddress)
{
	LogInfo("EPT Misconfiguration!");
	LogError("A field in the EPT paging structure was invalid, Faulting guest address : 0x%llx", GuestAddress);

	// We can't continue now. 
	// EPT misconfiguration is a fatal exception that will probably crash the OS if we don't get out now.
    // 现在我们无法继续。
    // 如果我们现在不退出，EPT配置错误将是一个致命异常，可能会导致操作系统崩溃。
}

/* Write an absolute x64 jump to an arbitrary address to a buffer. */
/* 将一个绝对的x64跳转指令写入到一个缓冲区中，跳转到任意地址。 */
VOID EptHookWriteAbsoluteJump(PCHAR TargetBuffer, SIZE_T TargetAddress)
{
	/* mov r15, Target */
	TargetBuffer[0] = 0x49;
	TargetBuffer[1] = 0xBB;

	/* Target */
	*((PSIZE_T)&TargetBuffer[2]) = TargetAddress;

	/* push r15 */
	TargetBuffer[10] = 0x41;
	TargetBuffer[11] = 0x53;

	/* ret */
	TargetBuffer[12] = 0xC3;
}


BOOLEAN EptHookInstructionMemory(PEPT_HOOKED_PAGE_DETAIL Hook, PVOID TargetFunction, PVOID HookFunction, PVOID* OrigFunction)
{
	SIZE_T SizeOfHookedInstructions;
	SIZE_T OffsetIntoPage;

	OffsetIntoPage = ADDRMASK_EPT_PML1_OFFSET((SIZE_T)TargetFunction);
	LogInfo("OffsetIntoPage: 0x%llx", OffsetIntoPage);

	if ((OffsetIntoPage + 13) > PAGE_SIZE - 1)
	{
		LogError("Function extends past a page boundary. We just don't have the technology to solve this.....");
		return FALSE;
	}

	/* Determine the number of instructions necessary to overwrite using Length Disassembler Engine */
    /* 使用长度解析引擎确定需要覆盖的指令数量。 */
	for (SizeOfHookedInstructions = 0;
		SizeOfHookedInstructions < 13;
		SizeOfHookedInstructions += LDE(TargetFunction, 64))
	{
		// Get the full size of instructions necessary to copy
        // 获取需要复制的指令的完整大小
	}

	LogInfo("Number of bytes of instruction mem: %d", SizeOfHookedInstructions);

	/* Build a trampoline */

	/* Allocate some executable memory for the trampoline */
    /* 构建一个跳板 */

    /* 分配一些可执行内存用于跳板 */
	Hook->Trampoline = PoolManagerRequestPool(EXEC_TRAMPOLINE, TRUE, MAX_EXEC_TRAMPOLINE_SIZE);

	if (!Hook->Trampoline)
	{
		LogError("Could not allocate trampoline function buffer.");
		return FALSE;
	}

	/* Copy the trampoline instructions in. */
    /* 将跳板指令复制到内存中。 */
	RtlCopyMemory(Hook->Trampoline, TargetFunction, SizeOfHookedInstructions);

	/* Add the absolute jump back to the original function. */
    /* 添加跳回原函数的绝对跳转指令。 */
	EptHookWriteAbsoluteJump(&Hook->Trampoline[SizeOfHookedInstructions], (SIZE_T)TargetFunction + SizeOfHookedInstructions);

	LogInfo("Trampoline: 0x%llx", Hook->Trampoline);
	LogInfo("HookFunction: 0x%llx", HookFunction);

	/* Let the hook function call the original function */
    /* 允许挂钩函数调用原始函数 */
	*OrigFunction = Hook->Trampoline;

	/* Write the absolute jump to our shadow page memory to jump to our hook. */
    /* 将绝对跳转指令写入到我们的影子页面内存，以跳转到我们的挂钩函数。 */
	EptHookWriteAbsoluteJump(&Hook->FakePageContents[OffsetIntoPage], (SIZE_T)HookFunction);

	return TRUE;
}

BOOLEAN EptHandleHookedPage(EPT_HOOKED_PAGE_DETAIL* HookedEntryDetails, VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification, SIZE_T PhysicalAddress) {

	ULONG64 GuestRip;
	ULONG64 ExactAccessedAddress;
	ULONG64 AlignedVirtualAddress;
	ULONG64 AlignedPhysicalAddress;


	// Get alignment
    // 获取对齐方式
	AlignedVirtualAddress = PAGE_ALIGN(HookedEntryDetails->VirtualAddress);
	AlignedPhysicalAddress = PAGE_ALIGN(PhysicalAddress);

	// Let's read the exact address that was accesses
    // 让我们读取被访问的确切地址
	ExactAccessedAddress = AlignedVirtualAddress + PhysicalAddress - AlignedPhysicalAddress;

	// Reading guest's RIP 
	// 读取客户机的RIP地址
	__vmx_vmread(GUEST_RIP, &GuestRip);

	if (!ViolationQualification.EptExecutable && ViolationQualification.ExecuteAccess)
	{
		LogInfo("Guest RIP : 0x%llx tries to execute the page at : 0x%llx", GuestRip, ExactAccessedAddress);

	}
	else if (!ViolationQualification.EptWriteable && ViolationQualification.WriteAccess)
	{
		LogInfo("Guest RIP : 0x%llx tries to write on the page at :0x%llx", GuestRip, ExactAccessedAddress);
	}
	else if (!ViolationQualification.EptReadable && ViolationQualification.ReadAccess)
	{
		LogInfo("Guest RIP : 0x%llx tries to read the page at :0x%llx", GuestRip, ExactAccessedAddress);
	}
	else
	{
		// there was an unexpected ept violation
        // 发生了意外的EPT违规事件
		return FALSE;
	}

	EptSetPML1AndInvalidateTLB(HookedEntryDetails->EntryAddress, HookedEntryDetails->OriginalEntry, INVEPT_SINGLE_CONTEXT);

	// Means that restore the Entry to the previous state after current instruction executed in the guest
    // 意味着在客户机中执行当前指令后，恢复条目到先前的状态
	return TRUE;
}

/* This function returns false in VMX Non-Root Mode if the VM is already initialized
   This function have to be called through a VMCALL in VMX Root Mode */
/* 如果VM已经初始化，则此函数在VMX非根模式下返回false。
必须通过VMX根模式下的VMCALL调用此函数。 */
BOOLEAN EptPerformPageHook(PVOID TargetAddress, PVOID HookFunction, PVOID* OrigFunction, BOOLEAN UnsetRead, BOOLEAN UnsetWrite, BOOLEAN UnsetExecute) {

	EPT_PML1_ENTRY ChangedEntry;
	INVEPT_DESCRIPTOR Descriptor;
	SIZE_T PhysicalAddress;
	PVOID VirtualTarget;
	PVOID TargetBuffer;
	PEPT_PML1_ENTRY TargetPage;
	PEPT_HOOKED_PAGE_DETAIL HookedPage;
	ULONG LogicalCoreIndex;

	// Check whether we are in VMX Root Mode or Not 
	// 检查我们是否处于VMX根模式下
	LogicalCoreIndex = KeGetCurrentProcessorIndex();

	if (GuestState[LogicalCoreIndex].IsOnVmxRootMode && !GuestState[LogicalCoreIndex].HasLaunched)
	{
		return FALSE;
	}

	/* Translate the page from a physical address to virtual so we can read its memory.
	 * This function will return NULL if the physical address was not already mapped in
	 * virtual memory.
	 */
    /* 将页面从物理地址转换为虚拟地址，以便我们可以读取其内存。
	如果物理地址在虚拟内存中尚未映射，则此函数将返回NULL。
	*/
	VirtualTarget = PAGE_ALIGN(TargetAddress);

	PhysicalAddress = (SIZE_T)VirtualAddressToPhysicalAddress(VirtualTarget);

	if (!PhysicalAddress)
	{
		LogError("Target address could not be mapped to physical memory");
		return FALSE;
	}

	// Set target buffer, request buffer from pool manager , we also need to allocate new page to replace the current page ASAP
    // 设置目标缓冲区，从池管理器请求缓冲区，我们还需要尽快分配新页面来替换当前页面
	TargetBuffer = PoolManagerRequestPool(SPLIT_2MB_PAGING_TO_4KB_PAGE, TRUE, sizeof(VMM_EPT_DYNAMIC_SPLIT));

	if (!TargetBuffer)
	{
		LogError("There is no pre-allocated buffer available");
		return FALSE;
	}

	if (!EptSplitLargePage(EptState->EptPageTable, TargetBuffer, PhysicalAddress, LogicalCoreIndex))
	{
		LogError("Could not split page for the address : 0x%llx", PhysicalAddress);
		return FALSE;
	}

	// Pointer to the page entry in the page table. 
	// 指向页表中的页面条目的指针。
	TargetPage = EptGetPml1Entry(EptState->EptPageTable, PhysicalAddress);

	// Ensure the target is valid. 
	// 确保目标是有效的。
	if (!TargetPage)
	{
		LogError("Failed to get PML1 entry of the target address");
		return FALSE;
	}

	// Save the original permissions of the page 
	// 保存页面的原始权限。
	ChangedEntry = *TargetPage;

	/* Execution is treated differently *
    /* 执行操作被单独处理 */

	if (UnsetRead)
		ChangedEntry.ReadAccess = 0;
	else
		ChangedEntry.ReadAccess = 1;

	if (UnsetWrite)
		ChangedEntry.WriteAccess = 0;
	else
		ChangedEntry.WriteAccess = 1;


	/* Save the detail of hooked page to keep track of it */
    /* 保存被挂钩页面的详细信息以便跟踪 */
	HookedPage = PoolManagerRequestPool(TRACKING_HOOKED_PAGES, TRUE, sizeof(EPT_HOOKED_PAGE_DETAIL));

	if (!HookedPage)
	{
		LogError("There is no pre-allocated pool for saving hooked page details");
		return FALSE;
	}

	// Save the virtual address
    // 保存虚拟地址
	HookedPage->VirtualAddress = TargetAddress;

	// Save the physical address
    // 保存物理地址
	HookedPage->PhysicalBaseAddress = PhysicalAddress;

	// Fake page content physical address
    // 假页面内容的物理地址
	HookedPage->PhysicalBaseAddressOfFakePageContents = (SIZE_T)VirtualAddressToPhysicalAddress(&HookedPage->FakePageContents[0]) / PAGE_SIZE;

	// Save the entry address
    // 保存条目的地址
	HookedPage->EntryAddress = TargetPage;

	// Save the orginal entry
    // 保存原始条目
	HookedPage->OriginalEntry = *TargetPage;


	// If it's Execution hook then we have to set extra fields
    // 如果是执行钩子，则需要设置额外的字段
	if (UnsetExecute)
	{
		// Show that entry has hidden hooks for execution
        // 表示该入口点存在用于执行的隐藏钩子
		HookedPage->IsExecutionHook = TRUE;

		// In execution hook, we have to make sure to unset read, write because
		// an EPT violation should occur for these cases and we can swap the original page
        // 在执行钩子中，我们必须确保取消读取和写入权限，因为这样可以在发生EPT违规时交换原始页面
		ChangedEntry.ReadAccess = 0;
		ChangedEntry.WriteAccess = 0;
		ChangedEntry.ExecuteAccess = 1;

		// Also set the current pfn to fake page
        // 同时将当前PFN设置为虚假页面的PFN
		ChangedEntry.PageFrameNumber = HookedPage->PhysicalBaseAddressOfFakePageContents;

		// Copy the content to the fake page
        // 将内容复制到虚假页面中
		RtlCopyBytes(&HookedPage->FakePageContents, VirtualTarget, PAGE_SIZE);

		// Create Hook
        // 创建钩子
		if (!EptHookInstructionMemory(HookedPage, TargetAddress, HookFunction, OrigFunction))
		{
			LogError("Could not build the hook.");
			return FALSE;
		}
	}

	// Save the modified entry
    // 保存修改后的页表项
	HookedPage->ChangedEntry = ChangedEntry;

	// Add it to the list 
	// 将其添加到列表中
	InsertHeadList(&EptState->HookedPagesList, &(HookedPage->PageHookList));

	/***********************************************************/
	// if not launched, there is no need to modify it on a safe environment
    // 如果未启动，则无需在安全环境下修改。
	if (!GuestState[LogicalCoreIndex].HasLaunched)
	{
		// Apply the hook to EPT 
		// 将钩子应用到 EPT 中。
		TargetPage->Flags = ChangedEntry.Flags;
	}
	else
	{
		// Apply the hook to EPT 
		// 将钩子应用到 EPT 中。
		EptSetPML1AndInvalidateTLB(TargetPage, ChangedEntry, INVEPT_SINGLE_CONTEXT);
	}

	return TRUE;
}


/*  This function allocates a buffer in VMX Non Root Mode and then invokes a VMCALL to set the hook */
/* 此函数在 VMX 非根模式下分配一个缓冲区，然后调用 VMCALL 来设置钩子。*/
BOOLEAN EptPageHook(PVOID TargetAddress, PVOID HookFunction, PVOID* OrigFunction, BOOLEAN SetHookForRead, BOOLEAN SetHookForWrite, BOOLEAN SetHookForExec) {

	ULONG LogicalProcCounts;
	PVOID PreAllocBuff;
	PVOID PagedAlignTarget;
	PEPT_HOOKED_PAGE_DETAIL HookedPageDetail;
	UINT32 PageHookMask = 0;
	ULONG LogicalCoreIndex;

	// Check for the features to avoid EPT Violation problems
    // 检查功能以避免 EPT 违规问题
	if (SetHookForExec && !ExecuteOnlySupport)
	{
		// In the current design of Hypervisor From Scratch we use execute-only pages to implement hidden hooks for exec page, 
		// so your processor doesn't have this feature and you have to implment it in other ways :(
		// 在Hypervisor From Scratch的当前设计中，我们使用执行权限为只执行（execute - only）的页面来实现隐藏的执行钩子。
		// 由于您的处理器不具备此功能，您需要使用其他方式来实现它。
		return FALSE;
	}

	if (SetHookForWrite && !SetHookForRead)
	{
		// The hidden hook with Write Enable and Read Disabled will cause EPT violation !!!
        // 具有写使能和读取禁用的隐藏钩子将导致EPT违规！！！
		return FALSE;
	}

	// Check whether we are in VMX Root Mode or Not 
	// 检查我们是否处于VMX根模式或非根模式
	LogicalCoreIndex = KeGetCurrentProcessorIndex();

	if (SetHookForRead)
	{
		PageHookMask |= PAGE_ATTRIB_READ;
	}
	if (SetHookForWrite)
	{
		PageHookMask |= PAGE_ATTRIB_WRITE;
	}
	if (SetHookForExec)
	{
		PageHookMask |= PAGE_ATTRIB_EXEC;
	}

	if (PageHookMask == 0)
	{
		// nothing to hook
        // 没有需要钩取的内容
		return FALSE;
	}

	if (GuestState[LogicalCoreIndex].HasLaunched)
	{
		// Move Attribute Mask to the upper 32 bits of the VMCALL Number 
		// 将属性掩码移动到VMCALL号码的高32位
		UINT64 VmcallNumber = ((UINT64)PageHookMask) << 32 | VMCALL_CHANGE_PAGE_ATTRIB;

		if (AsmVmxVmcall(VmcallNumber, TargetAddress, HookFunction, OrigFunction) == STATUS_SUCCESS)
		{
			LogInfo("Hook applied from VMX Root Mode");
			if (!GuestState[LogicalCoreIndex].IsOnVmxRootMode)
			{
				// Now we have to notify all the core to invalidate their EPT
                // 现在我们必须通知所有核心使其无效化其EPT
				HvNotifyAllToInvalidateEpt();

			}
			else
			{
				LogError("Unable to notify all cores to invalidate their TLB caches as you called hook on vmx-root mode.");
			}

			return TRUE;
		}
	}
	else
	{
		if (EptPerformPageHook(TargetAddress, HookFunction, OrigFunction, SetHookForRead, SetHookForWrite, SetHookForExec) == TRUE) {
			LogInfo("[*] Hook applied (VM has not launched)");
			return TRUE;
		}
	}
	LogWarning("Hook not applied");

	return FALSE;
}

/*  This function set the specific PML1 entry in a spinlock protected area then invalidate the TLB ,
	this function should be called from vmx root-mode
*/
/* 此函数在受自旋锁保护的区域内设置特定的PML1条目，然后使TLB无效化，
此函数应该从VMX根模式中调用 */
VOID EptSetPML1AndInvalidateTLB(PEPT_PML1_ENTRY EntryAddress, EPT_PML1_ENTRY EntryValue, INVEPT_TYPE InvalidationType)
{
	// acquire the lock

    // 获取锁
	SpinlockLock(&Pml1ModificationAndInvalidationLock);

	// set the value
    // 设置值
	EntryAddress->Flags = EntryValue.Flags;

	// invalidate the cache
    // 使缓存无效化
	if (InvalidationType == INVEPT_SINGLE_CONTEXT)
	{
		InveptSingleContext(EptState->EptPointer.Flags);
	}
	else
	{
		InveptAllContexts();
	}
	// release the lock
    // 释放锁
	SpinlockUnlock(&Pml1ModificationAndInvalidationLock);
}

/* Remove and Invalidate Hook in TLB */
// Caution : This function won't remove entries from LIST_ENTRY, just invalidate the paging, use HvPerformPageUnHookSinglePage instead

/* 从TLB中移除并使钩子失效 */
// 注意：此函数不会从LIST_ENTRY中移除条目，仅使分页失效，请使用HvPerformPageUnHookSinglePage函数代替
BOOLEAN EptPageUnHookSinglePage(SIZE_T PhysicalAddress) {
	PLIST_ENTRY TempList = 0;

	// Should be called from vmx-root, for calling from vmx non-root use the corresponding VMCALL
    // 应该从VMX根模式中调用，对于从VMX非根模式调用，请使用相应的VMCALL。
	if (!GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
	{
		return FALSE;
	}

	TempList = &EptState->HookedPagesList;
	while (&EptState->HookedPagesList != TempList->Flink)
	{
		TempList = TempList->Flink;
		PEPT_HOOKED_PAGE_DETAIL HookedEntry = CONTAINING_RECORD(TempList, EPT_HOOKED_PAGE_DETAIL, PageHookList);
		if (HookedEntry->PhysicalBaseAddress == PAGE_ALIGN(PhysicalAddress))
		{
			// Undo the hook on the EPT table
            // 撤销对EPT表的钩取
			EptSetPML1AndInvalidateTLB(HookedEntry->EntryAddress, HookedEntry->OriginalEntry, INVEPT_SINGLE_CONTEXT);
			return TRUE;
		}
	}
	// Nothing found , probably the list is not found
    // 未找到任何内容，可能列表未找到。
	return FALSE;
}

/* Remove and Invalidate Hook in TLB */
// Caution : This function won't remove entries from LIST_ENTRY, just invalidate the paging, use HvPerformPageUnHookAllPages instead
/* 从TLB中移除并使钩子失效 */
// 注意：此函数不会从LIST_ENTRY中移除条目，只会使分页失效，请改用HvPerformPageUnHookAllPages函数
VOID EptPageUnHookAllPages() {
	PLIST_ENTRY TempList = 0;

	// Should be called from vmx-root, for calling from vmx non-root use the corresponding VMCALL
    // 应该从VMX根模式中调用，对于从VMX非根模式调用，请使用相应的VMCALL。
	if (!GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
	{
		return FALSE;
	}

	TempList = &EptState->HookedPagesList;
	while (&EptState->HookedPagesList != TempList->Flink)
	{
		TempList = TempList->Flink;
		PEPT_HOOKED_PAGE_DETAIL HookedEntry = CONTAINING_RECORD(TempList, EPT_HOOKED_PAGE_DETAIL, PageHookList);

		// Undo the hook on the EPT table
        // 撤销对EPT表的钩取。
		EptSetPML1AndInvalidateTLB(HookedEntry->EntryAddress, HookedEntry->OriginalEntry, INVEPT_SINGLE_CONTEXT);
	}
}
