#pragma once



//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////

// MTRR Physical Base MSRs
// MTRR 物理基址 MSR
#define MSR_IA32_MTRR_PHYSBASE0                                          0x00000200
#define MSR_IA32_MTRR_PHYSBASE1                                          0x00000202
#define MSR_IA32_MTRR_PHYSBASE2                                          0x00000204
#define MSR_IA32_MTRR_PHYSBASE3                                          0x00000206
#define MSR_IA32_MTRR_PHYSBASE4                                          0x00000208
#define MSR_IA32_MTRR_PHYSBASE5                                          0x0000020A
#define MSR_IA32_MTRR_PHYSBASE6                                          0x0000020C
#define MSR_IA32_MTRR_PHYSBASE7                                          0x0000020E
#define MSR_IA32_MTRR_PHYSBASE8                                          0x00000210
#define MSR_IA32_MTRR_PHYSBASE9                                          0x00000212

// MTRR Physical Mask MSRs
// MTRR 物理掩码 MSR
#define MSR_IA32_MTRR_PHYSMASK0                                          0x00000201
#define MSR_IA32_MTRR_PHYSMASK1                                          0x00000203
#define MSR_IA32_MTRR_PHYSMASK2                                          0x00000205
#define MSR_IA32_MTRR_PHYSMASK3                                          0x00000207
#define MSR_IA32_MTRR_PHYSMASK4                                          0x00000209
#define MSR_IA32_MTRR_PHYSMASK5                                          0x0000020B
#define MSR_IA32_MTRR_PHYSMASK6                                          0x0000020D
#define MSR_IA32_MTRR_PHYSMASK7                                          0x0000020F
#define MSR_IA32_MTRR_PHYSMASK8                                          0x00000211
#define MSR_IA32_MTRR_PHYSMASK9                                          0x00000213


// Memory Types
// 内存类型
#define MEMORY_TYPE_UNCACHEABLE                                      0x00000000
#define MEMORY_TYPE_WRITE_COMBINING                                  0x00000001
#define MEMORY_TYPE_WRITE_THROUGH                                    0x00000004
#define MEMORY_TYPE_WRITE_PROTECTED                                  0x00000005
#define MEMORY_TYPE_WRITE_BACK                                       0x00000006
#define MEMORY_TYPE_INVALID                                          0x000000FF


// Page attributes for internal use
// 用于内部使用的页面属性
#define PAGE_ATTRIB_READ											 0x2       
#define PAGE_ATTRIB_WRITE											 0x4       
#define PAGE_ATTRIB_EXEC											 0x8       


// VMX EPT & VPID Capabilities MSR
// VMX EPT 和 VPID 能力 MSR
#define MSR_IA32_VMX_EPT_VPID_CAP                                        0x0000048C

// MTRR Def MSR
// MTRR 默认 MSR
#define MSR_IA32_MTRR_DEF_TYPE                                           0x000002FF

// MTRR Capabilities MSR
// MTRR 能力 MSR
#define MSR_IA32_MTRR_CAPABILITIES                                       0x000000FE

// The number of 512GB PML4 entries in the page table/
// 页表中 512GB PML4 条目的数量
#define VMM_EPT_PML4E_COUNT 512

// The number of 1GB PDPT entries in the page table per 512GB PML4 entry.
// 每个 512GB PML4 条目中页表中 1GB PDPT 条目的数量
#define VMM_EPT_PML3E_COUNT 512

// Then number of 2MB Page Directory entries in the page table per 1GB PML3 entry.
// 每个 1GB PML3 条目中页表中 2MB Page Directory 条目的数量
#define VMM_EPT_PML2E_COUNT 512

// Then number of 4096 byte Page Table entries in the page table per 2MB PML2 entry when dynamically split.
// 当动态拆分时，每个 2MB PML2 条目中页表中 4096 字节 Page Table 条目的数量
#define VMM_EPT_PML1E_COUNT 512

// Integer 2MB
// 整数 2MB
#define SIZE_2_MB ((SIZE_T)(512 * PAGE_SIZE))

// Offset into the 1st paging structure (4096 byte)
// 偏移量进入第一个分页结构（4096 字节）
#define ADDRMASK_EPT_PML1_OFFSET(_VAR_) (_VAR_ & 0xFFFULL)

// Index of the 1st paging structure (4096 byte)
// 第一个分页结构（4096 字节）的索引
#define ADDRMASK_EPT_PML1_INDEX(_VAR_) ((_VAR_ & 0x1FF000ULL) >> 12)

// Index of the 2nd paging structure (2MB)
// 第二个分页结构（2MB）的索引
#define ADDRMASK_EPT_PML2_INDEX(_VAR_) ((_VAR_ & 0x3FE00000ULL) >> 21)

// Index of the 3rd paging structure (1GB)
// 第三个分页结构（1GB）的索引
#define ADDRMASK_EPT_PML3_INDEX(_VAR_) ((_VAR_ & 0x7FC0000000ULL) >> 30)

// Index of the 4th paging structure (512GB)
// 第四个分页结构（512GB）的索引
#define ADDRMASK_EPT_PML4_INDEX(_VAR_) ((_VAR_ & 0xFF8000000000ULL) >> 39)

/**
 * Linked list for-each macro for traversing LIST_ENTRY structures.
 *
 * _LISTHEAD_ is a pointer to the struct that the list head belongs to.
 * _LISTHEAD_NAME_ is the name of the variable which contains the list head. Should match the same name as the list entry struct member in the actual record.
 * _TARGET_TYPE_ is the type name of the struct of each item in the list
 * _TARGET_NAME_ is the name which will contain the pointer to the item each iteration
 *
 * Example:
 * FOR_EACH_LIST_ENTRY(ProcessorContext->EptPageTable, DynamicSplitList, VMM_EPT_DYNAMIC_SPLIT, Split)
 * 		OsFreeNonpagedMemory(Split);
 * }
 *
 * ProcessorContext->EptPageTable->DynamicSplitList is the head of the list.
 * VMM_EPT_DYNAMIC_SPLIT is the struct of each item in the list.
 * Split is the name of the local variable which will hold the pointer to the item.
 */

/**
用于遍历 LIST_ENTRY 结构的链表遍历宏。
LISTHEAD 是指向包含列表头的结构的指针。
LISTHEAD_NAME 是包含列表头的变量的名称。应该与实际记录中列表入口结构成员的名称匹配。
TARGET_TYPE 是列表中每个项的结构的类型名称。
TARGET_NAME 是每次迭代中将包含项指针的变量的名称。
示例：
FOR_EACH_LIST_ENTRY(ProcessorContext->EptPageTable, DynamicSplitList, VMM_EPT_DYNAMIC_SPLIT, Split)
 OsFreeNonpagedMemory(Split);
}
ProcessorContext->EptPageTable->DynamicSplitList 是列表的头部。
VMM_EPT_DYNAMIC_SPLIT 是列表中每个项的结构。
Split 是将保存项指针的局部变量的名称。
*/
#define FOR_EACH_LIST_ENTRY(_LISTHEAD_, _LISTHEAD_NAME_, _TARGET_TYPE_, _TARGET_NAME_) \
	for (PLIST_ENTRY Entry = _LISTHEAD_->_LISTHEAD_NAME_.Flink; Entry != &_LISTHEAD_->_LISTHEAD_NAME_; Entry = Entry->Flink) { \
	P##_TARGET_TYPE_ _TARGET_NAME_ = CONTAINING_RECORD(Entry, _TARGET_TYPE_, _LISTHEAD_NAME_);

 /**
  * The braces for the block are messy due to the need to define a local variable in the for loop scope.
  * Therefore, this macro just ends the for each block without messing up code editors trying to detect
  * the block indent level.
  */

/**
由于需要在 for 循环作用域中定义局部变量，因此该宏中的大括号会显得混乱。
因此，该宏只是结束 for each 块，而不会破坏代码编辑器试图检测块缩进级别的功能。
*/
# define FOR_EACH_LIST_ENTRY_END() }

  //////////////////////////////////////////////////
  //	    			Variables 		    	  //
  //////////////////////////////////////////////////

// Vmx-root lock for changing EPT PML1 Entry and Invalidating TLB
// 更改 EPT PML1 条目和使 TLB 失效的 Vmx-root 锁
volatile LONG Pml1ModificationAndInvalidationLock;



//////////////////////////////////////////////////
//				Unions & Structs    			//
//////////////////////////////////////////////////

typedef union _IA32_VMX_EPT_VPID_CAP_REGISTER
{
	struct
	{
		/**
		 * [Bit 0] When set to 1, the processor supports execute-only translations by EPT. This support allows software to
		 * configure EPT paging-structure entries in which bits 1:0 are clear (indicating that data accesses are not allowed) and
		 * bit 2 is set (indicating that instruction fetches are allowed).
		 */

        /**
		 * [位 0] 当设置为 1 时，处理器支持 EPT 的只执行转换。该支持允许软件配置 EPT 分页结构条目，其中位 1:0 为 0（表示不允许数据访问），
		 * 并且位 2 为 1（表示允许指令获取）。
		 */
		UINT64 ExecuteOnlyPages : 1;
		UINT64 Reserved1 : 5;

		/**
		 * [Bit 6] Indicates support for a page-walk length of 4.
		 */

        /**
		* [位 6] 表示支持页级别为 4。
		*/
		UINT64 PageWalkLength4 : 1;
		UINT64 Reserved2 : 1;

		/**
		 * [Bit 8] When set to 1, the logical processor allows software to configure the EPT paging-structure memory type to be
		 * uncacheable (UC).
		 *
		 * @see Vol3C[24.6.11(Extended-Page-Table Pointer (EPTP))]
		 */

        /**
		* [位 8] 当设置为 1 时，逻辑处理器允许软件将 EPT 分页结构的内存类型配置为不可缓存 (UC)。
		*
		* @see Vol3C[24.6.11(Extended-Page-Table Pointer (EPTP))]
		*/
		UINT64 MemoryTypeUncacheable : 1;
		UINT64 Reserved3 : 5;

		/**
		 * [Bit 14] When set to 1, the logical processor allows software to configure the EPT paging-structure memory type to be
		 * write-back (WB).
		 */
		
		/**
		 * [位 14] 当设置为 1 时，逻辑处理器允许软件将 EPT 分页结构的内存类型配置为写回 (WB)。
		 */
		UINT64 MemoryTypeWriteBack : 1;
		UINT64 Reserved4 : 1;

		/**
		 * [Bit 16] When set to 1, the logical processor allows software to configure a EPT PDE to map a 2-Mbyte page (by setting
		 * bit 7 in the EPT PDE).
		 */

		/**
		* [位 16] 当设置为 1 时，逻辑处理器允许软件配置 EPT PDE 来映射一个 2MB 页（通过设置 EPT PDE 中的位 7）。
		*/
		UINT64 Pde2MbPages : 1;

		/**
		 * [Bit 17] When set to 1, the logical processor allows software to configure a EPT PDPTE to map a 1-Gbyte page (by setting
		 * bit 7 in the EPT PDPTE).
		 */

        /**
		* [位 17] 当设置为 1 时，逻辑处理器允许软件配置 EPT PDPTE 来映射一个 1GB 页（通过设置 EPT PDPTE 中的位 7）。
		*/
		UINT64 Pdpte1GbPages : 1;
		UINT64 Reserved5 : 2;

		/**
		 * [Bit 20] If bit 20 is read as 1, the INVEPT instruction is supported.
		 *
		 * @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
		 * @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
		 */

        /**
		* [位 20] 如果读取为 1，则支持 INVEPT 指令。
		*
		* @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
		* @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
		*/
		UINT64 Invept : 1;

		/**
		 * [Bit 21] When set to 1, accessed and dirty flags for EPT are supported.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

        /**
		* [位 21] 当设置为 1 时，支持 EPT 的访问标志和脏标志。
		*
		* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		*/
		UINT64 EptAccessedAndDirtyFlags : 1;

		/**
		 * [Bit 22] When set to 1, the processor reports advanced VM-exit information for EPT violations. This reporting is done
		 * only if this bit is read as 1.
		 *
		 * @see Vol3C[27.2.1(Basic VM-Exit Information)]
		 */

		/**
		* [位 22] 当设置为 1 时，处理器对于 EPT 违规报告高级 VM-exit 信息。仅当此位读取为 1 时才进行报告。
		*
		* @see Vol3C[27.2.1(Basic VM-Exit Information)]
		*/
		UINT64 AdvancedVmexitEptViolationsInformation : 1;
		UINT64 Reserved6 : 2;

		/**
		 * [Bit 25] When set to 1, the single-context INVEPT type is supported.
		 *
		 * @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
		 * @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
		 */

		/**
		* [位 25] 当设置为 1 时，支持单上下文 INVEPT 类型。
		*
		* @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
		* @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
		*/
		UINT64 InveptSingleContext : 1;

		/**
		 * [Bit 26] When set to 1, the all-context INVEPT type is supported.
		 *
		 * @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
		 * @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
		 */

		/**
		* [位 26] 当设置为 1 时，支持全上下文 INVEPT 类型。
		*
		* @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
		* @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
		*/
		UINT64 InveptAllContexts : 1;
		UINT64 Reserved7 : 5;

		/**
		 * [Bit 32] When set to 1, the INVVPID instruction is supported.
		 */

		/**
		* [位 32] 当设置为 1 时，支持 INVVPID 指令。
		*/
		UINT64 Invvpid : 1;
		UINT64 Reserved8 : 7;

		/**
		 * [Bit 40] When set to 1, the individual-address INVVPID type is supported.
		 */

		/**
		* [位 40] 当设置为 1 时，支持单地址 INVVPID 类型。
		*/
		UINT64 InvvpidIndividualAddress : 1;

		/**
		 * [Bit 41] When set to 1, the single-context INVVPID type is supported.
		 */

        /**
		* [位 41] 当设置为 1 时，支持单上下文 INVVPID 类型。
		*/
		UINT64 InvvpidSingleContext : 1;

		/**
		 * [Bit 42] When set to 1, the all-context INVVPID type is supported.
		 */

		/**
		* [位 42] 当设置为 1 时，支持全上下文 INVVPID 类型。
		*/
		UINT64 InvvpidAllContexts : 1;

		/**
		 * [Bit 43] When set to 1, the single-context-retaining-globals INVVPID type is supported.
		 */

		/**
		 * [位 43] 当设置为 1 时，支持保留全局的单上下文 INVVPID 类型。
		 */
		UINT64 InvvpidSingleContextRetainGlobals : 1;
		UINT64 Reserved9 : 20;
	};

	UINT64 Flags;
} IA32_VMX_EPT_VPID_CAP_REGISTER, * PIA32_VMX_EPT_VPID_CAP_REGISTER;


typedef union _PEPT_PML4
{
	struct
	{
		/**
		 * [Bit 0] Read access; indicates whether reads are allowed from the 512-GByte region controlled by this entry.
		 */

		/**
		* [位 0] 读访问；指示是否允许从由该条目控制的 512GB 区域进行读取。
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 512-GByte region controlled by this entry.
		 */

		/**
		* [位 1] 写访问；指示是否允许从由该条目控制的 512GB 区域进行写入。
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 512-GByte region controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 512-GByte region controlled by this entry.
		 */

		/**
		* [位 2] 如果 "mode-based execute control for EPT" VM-execution 控制位为 0，表示执行访问权限；指示是否允许从由该条目控制的 512GB 区域进行指令获取。
		* 如果该控制位为 1，则表示超级模式线性地址的执行访问权限；指示是否允许从由该条目控制的 512GB 区域中的超级模式线性地址进行指令获取。
		*/
		UINT64 ExecuteAccess : 1;
		UINT64 Reserved1 : 5;

		/**
		 * [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 512-GByte region
		 * controlled by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [位 8] 如果 EPTP 的位 6 为 1，则表示 EPT 的访问标志；指示软件是否已访问由该条目控制的 512GB 区域。如果 EPTP 的位 6 为 0，则忽略该标志。
		*
		* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		*/
		UINT64 Accessed : 1;
		UINT64 Reserved2 : 1;

		/**
		 * [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 512-GByte region
		 * controlled by this entry. If that control is 0, this bit is ignored.
		 */

		/**
		* [位 10] 用户模式线性地址的执行访问权限。如果 "mode-based execute control for EPT" VM-execution 控制位为 1，则表示是否允许从由该条目控制的 512GB 区域中的用户模式线性地址进行指令获取。如果该控制位为 0，则忽略该标志。
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved3 : 1;

		/**
		 * [Bits 47:12] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */

		/**
		* [位 47:12] 该条目引用的按 4KB 对齐的 EPT 页目录指针表的物理地址。
		*/
		UINT64 PageFrameNumber : 36;
		UINT64 Reserved4 : 16;
	};

	UINT64 Flags;
} EPT_PML4, * PEPT_PML4;



typedef union _EPDPTE_1GB
{
	struct
	{
		/**
		 * [Bit 0] Read access; indicates whether reads are allowed from the 1-GByte page referenced by this entry.
		 */

		/**
		* [位 0] 读访问权限；指示是否允许从由该条目引用的 1GB 页进行读取。
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 1-GByte page referenced by this entry.
		 */

        /**
		* [位 1] 写访问权限；指示是否允许从由该条目引用的 1GB 页进行写入。
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 1-GByte page controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 1-GByte page controlled by this entry.
		 */

		/**
		* [位 2] 如果 "mode-based execute control for EPT" VM-execution 控制位为 0，执行访问权限；指示是否允许从由该条目控制的 1GB 页进行指令获取。
		* 如果该控制位为 1，则表示超级模式线性地址的执行访问权限；指示是否允许从由该条目控制的 1GB 页中的超级模式线性地址进行指令获取。
		*/
		UINT64 ExecuteAccess : 1;

		/**
		 * [Bits 5:3] EPT memory type for this 1-GByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

        /**
		* [位 5:3] 该 1GB 页的 EPT 内存类型。
		*
		* @see Vol3C[28.2.6(EPT and memory Typing)]
		*/
		UINT64 MemoryType : 3;

		/**
		 * [Bit 6] Ignore PAT memory type for this 1-GByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		* [位 6] 忽略此 1GB 页的 PAT 内存类型。
		*
		* @see Vol3C[28.2.6(EPT and memory Typing)]
		*/
		UINT64 IgnorePat : 1;

		/**
		 * [Bit 7] Must be 1 (otherwise, this entry references an EPT page directory).
		 */

		/**
		* [位 7] 必须为 1（否则，该条目引用的是 EPT 页目录）。
		*/
		UINT64 LargePage : 1;

		/**
		 * [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 1-GByte page
		 * referenced by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [位 8] 如果 EPTP 的位 6 为 1，则为 EPT 的访问标志；指示软件是否访问了该条目引用的 1GB 页。如果 EPTP 的位 6 为 0，则忽略此标志。
		*
		* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		*/
		UINT64 Accessed : 1;

		/**
		 * [Bit 9] If bit 6 of EPTP is 1, dirty flag for EPT; indicates whether software has written to the 1-GByte page referenced
		 * by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [位 9] 如果 EPTP 的位 6 为 1，则为 EPT 的脏位；指示软件是否对该条目引用的 1GB 页进行了写入。如果 EPTP 的位 6 为 0，则忽略此标志。
		*
		* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		*/
		UINT64 Dirty : 1;

		/**
		 * [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 1-GByte page controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */

		/**
		* [位 10] 用户模式线性地址的执行访问权限。如果 "基于模式的 EPT 执行控制" VM 执行控制为 1，则表示是否允许从此条目所控制的 1GB 页中的用户模式线性地址进行指令获取。
		* 如果该控制为 0，则忽略此位。
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved1 : 19;

		/**
		 * [Bits 47:30] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */

		/**
		* [位 47:30] 此条目引用的 4KB 对齐的 EPT 页目录指针表的物理地址。
		*/
		UINT64 PageFrameNumber : 18;
		UINT64 Reserved2 : 15;

		/**
		 * [Bit 63] Suppress \#VE. If the "EPT-violation \#VE" VM-execution control is 1, EPT violations caused by accesses to this
		 * page are convertible to virtualization exceptions only if this bit is 0. If "EPT-violation \#VE" VMexecution control is
		 * 0, this bit is ignored.
		 *
		 * @see Vol3C[25.5.6.1(Convertible EPT Violations)]
		 */

		/**
		* [位 63] 抑制 #VE。如果 "EPT 违规 #VE" VM 执行控制为 1，则仅当此位为 0 时，对该页的访问引起的 EPT 违规才能转换为虚拟化异常。
		* 如果 "EPT 违规 #VE" VM 执行控制为 0，则忽略此位。
		*
		* @see Vol3C[25.5.6.1(可转换的 EPT 违规)]
		*/
		UINT64 SuppressVe : 1;
	};

	UINT64 Flags;
} EPDPTE_1GB, * PEPDPTE_1GB;



typedef union _EPDPTE
{
	struct
	{
		/**
		 * [Bit 0] Read access; indicates whether reads are allowed from the 1-GByte region controlled by this entry.
		 */

        /**
		* [位 0] 读访问权限；指示是否允许从受此条目控制的 1-GByte 区域进行读取。
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 1-GByte region controlled by this entry.
		 */

		/**
		* [位 1] 写访问权限；指示是否允许从受此条目控制的 1-GByte 区域进行写入。
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 1-GByte region controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 1-GByte region controlled by this entry.
		 */

		/**
		* [位 2] 如果 "基于模式的执行控制用于 EPT" VM-execution 控制位为 0，则执行访问权限；指示是否允许从受此条目控制的 1-GByte 区域进行指令获取。
		* 如果该控制位为 1，则表示针对特权级线性地址的执行访问权限；指示是否允许从受此条目控制的 1-GByte 区域中的特权级线性地址进行指令获取。
		*/
		UINT64 ExecuteAccess : 1;
		UINT64 Reserved1 : 5;

		/**
		 * [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 1-GByte region
		 * controlled by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [位 8] 如果 EPTP 的第 6 位为 1，则表示用于 EPT 的已访问标志；指示软件是否已访问受此条目控制的 1-GByte 区域。如果 EPTP 的第 6 位为 0，则忽略此标志。
		*
		* @see Vol3C[28.2.4(EPT 的已访问和脏位)]
		*/
		UINT64 Accessed : 1;
		UINT64 Reserved2 : 1;

		/**
		 * [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 1-GByte region controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */

		/**
		* [位 10] 用户模式线性地址的执行访问权限。如果 "基于模式的执行控制用于 EPT" VM-execution 控制位为 1，则表示是否允许从受此条目控制的 1-GByte 区域中的用户模式线性地址进行指令获取。如果该控制位为 0，则忽略此位。
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved3 : 1;

		/**
		 * [Bits 47:12] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */

		/**
		* [位 47:12] 引用此条目的 4-KByte 对齐的 EPT 页目录指针表的物理地址。
		*/
		UINT64 PageFrameNumber : 36;
		UINT64 Reserved4 : 16;
	};

	UINT64 Flags;
} EPDPTE, * PEPDPTE;


typedef union _EPDE_2MB
{
	struct
	{
		/**
		 * [Bit 0] Read access; indicates whether reads are allowed from the 2-MByte page referenced by this entry.
		 */

		/**
		* [位 0] 读取访问权限；指示是否允许从此条目引用的 2-MByte 页进行读取。
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 2-MByte page referenced by this entry.
		 */

		/**
		* [位 1] 写入访问权限；指示是否允许从此条目引用的 2-MByte 页进行写入。
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 2-MByte page controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 2-MByte page controlled by this entry.
		 */

		/**
		* [位 2] 如果 "基于模式的执行控制用于 EPT" VM-execution 控制位为 0，则执行访问权限；指示是否允许从受此条目控制的 2-MByte 页进行指令获取。
		* 如果该控制位为 1，则表示针对特权级线性地址的执行访问权限；指示是否允许从受此条目控制的 2-MByte 页中的特权级线性地址进行指令获取。
		*/
		UINT64 ExecuteAccess : 1;

		/**
		 * [Bits 5:3] EPT memory type for this 2-MByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		 * [位 5:3] 此 2-MByte 页的 EPT 内存类型。
		 *
		 * @see Vol3C[28.2.6(EPT 和内存类型)]
		 */
		UINT64 MemoryType : 3;

		/**
		 * [Bit 6] Ignore PAT memory type for this 2-MByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		 * [位 6] 忽略此 2-MByte 页的 PAT 内存类型。
		 *
		 * @see Vol3C[28.2.6(EPT 和内存类型)]
		 */
		UINT64 IgnorePat : 1;

		/**
		 * [Bit 7] Must be 1 (otherwise, this entry references an EPT page table).
		 */

		/**
		* [位 7] 必须为 1（否则，此条目引用的是 EPT 页表）。
		*/
		UINT64 LargePage : 1;

		/**
		 * [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 2-MByte page
		 * referenced by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [位 8] 如果 EPTP 的第 6 位为 1，则表示用于 EPT 的已访问标志；指示软件是否已访问受此条目控制的 2-MByte 页。如果 EPTP 的第 6 位为 0，则忽略此标志。
		*
		* @see Vol3C[28.2.4(EPT 的已访问和脏位)]
		*/
		UINT64 Accessed : 1;

		/**
		 * [Bit 9] If bit 6 of EPTP is 1, dirty flag for EPT; indicates whether software has written to the 2-MByte page referenced
		 * by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [位 9] 如果 EPTP 的第 6 位为 1，则表示用于 EPT 的脏位；指示软件是否已对受此条目控制的 2-MByte 页进行写入。如果 EPTP 的第 6 位为 0，则忽略此标志。
		*
		* @see Vol3C[28.2.4(EPT 的已访问和脏位)]
		*/
		UINT64 Dirty : 1;

		/**
		 * [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 2-MByte page controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */

		/**
		* [位 10] 用户模式线性地址的执行访问权限。如果 "基于模式的执行控制用于 EPT" VM-execution 控制位为 1，则表示是否允许从受此条目控制的 2-MByte 页中的用户模式线性地址进行指令获取。如果该控制位为 0，则忽略此位。
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved1 : 10;

		/**
		 * [Bits 47:21] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */

		/**
		* [位 47:21] 引用此条目的 4-KByte 对齐的 EPT 页目录指针表的物理地址。
		*/
		UINT64 PageFrameNumber : 27;
		UINT64 Reserved2 : 15;

		/**
		 * [Bit 63] Suppress \#VE. If the "EPT-violation \#VE" VM-execution control is 1, EPT violations caused by accesses to this
		 * page are convertible to virtualization exceptions only if this bit is 0. If "EPT-violation \#VE" VMexecution control is
		 * 0, this bit is ignored.
		 *
		 * @see Vol3C[25.5.6.1(Convertible EPT Violations)]
		 */

		/**
		* [位 63] 抑制 #VE。如果 "EPT-violation #VE" VM-execution 控制位为 1，则仅当此位为 0 时，由于访问此页面而导致的 EPT 违规可以转换为虚拟化异常。如果 "EPT-violation #VE" VM-execution 控制位为 0，则忽略此位。
		*
		* @see Vol3C[25.5.6.1(可转换的 EPT 违规)]
		*/
		UINT64 SuppressVe : 1;
	};

	UINT64 Flags;
} EPDE_2MB, * PEPDE_2MB;



typedef union _EPDE
{
	struct
	{
		/**
		 * [Bit 0] Read access; indicates whether reads are allowed from the 2-MByte region controlled by this entry.
		 */

		/**
		* [位 0] 读取访问权限；指示是否允许从受此条目控制的 2-MByte 区域进行读取。
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 2-MByte region controlled by this entry.
		 */

		/**
		* [位 1] 写入访问权限；指示是否允许从受此条目控制的 2-MByte 区域进行写入。
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 2-MByte region controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 2-MByte region controlled by this entry.
		 */

		/**
		* [位 2] 如果 "基于模式的执行控制用于 EPT" VM-execution 控制位为 0，则执行访问权限；指示是否允许从受此条目控制的 2-MByte 区域进行指令获取。
		* 如果该控制位为 1，则表示针对特权级线性地址的执行访问权限；指示是否允许从受此条目控制的 2-MByte 区域中的特权级线性地址进行指令获取。
		*/
		UINT64 ExecuteAccess : 1;
		UINT64 Reserved1 : 5;

		/**
		 * [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 2-MByte region
		 * controlled by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [位 8] 如果 EPTP 的第 6 位为 1，则表示用于 EPT 的已访问标志；指示软件是否已访问受此条目控制的 2-MByte 区域。如果 EPTP 的第 6 位为 0，则忽略此标志。
		*
		* @see Vol3C[28.2.4(EPT 的已访问和脏位)]
		*/
		UINT64 Accessed : 1;
		UINT64 Reserved2 : 1;

		/**
		 * [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 2-MByte region controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */

		/**
		* [位 10] 用户模式线性地址的执行访问权限。如果 "基于模式的执行控制用于 EPT" VM-execution 控制位为 1，则表示是否允许从受此条目控制的 2-MByte 区域中的用户模式线性地址进行指令获取。如果该控制位为 0，则忽略此位。
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved3 : 1;

		/**
		 * [Bits 47:12] Physical address of 4-KByte aligned EPT page table referenced by this entry.
		 */

		/**
		* [位 47:12] 引用此条目的 4-KByte 对齐的 EPT 页表的物理地址。
		*/
		UINT64 PageFrameNumber : 36;
		UINT64 Reserved4 : 16;
	};

	UINT64 Flags;
} EPDE, * PEPDE;


typedef union _EPTE
{
	struct
	{
		/**
		 * [Bit 0] Read access; indicates whether reads are allowed from the 4-KByte page referenced by this entry.
		 */

		/**
		* [位 0] 读取访问权限；指示是否允许从此条目引用的 4-KByte 页进行读取。
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 4-KByte page referenced by this entry.
		 */

		/**
		* [位 1] 写入访问权限；指示是否允许从此条目引用的 4-KByte 页进行写入。
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 4-KByte page controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 4-KByte page controlled by this entry.
		 */

		/**
		* [位 2] 如果 "基于模式的执行控制用于 EPT" VM-execution 控制位为 0，则执行访问权限；指示是否允许从受此条目控制的 4-KByte 页进行指令获取。
		* 如果该控制位为 1，则表示针对特权级线性地址的执行访问权限；指示是否允许从受此条目控制的 4-KByte 页中的特权级线性地址进行指令获取。
		*/
		UINT64 ExecuteAccess : 1;

		/**
		 * [Bits 5:3] EPT memory type for this 4-KByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		* [位 5:3] 此 4-KByte 页的 EPT 内存类型。
		*
		* @see Vol3C[28.2.6(EPT 和内存类型)]
		*/
		UINT64 MemoryType : 3;

		/**
		 * [Bit 6] Ignore PAT memory type for this 4-KByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		* [位 6] 忽略此 4-KByte 页的 PAT 内存类型。
		*
		* @see Vol3C[28.2.6(EPT 和内存类型)]
		*/
		UINT64 IgnorePat : 1;
		UINT64 Reserved1 : 1;

		/**
		 * [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 4-KByte page
		 * referenced by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [位 8] 如果 EPTP 的第 6 位为 1，则表示用于 EPT 的已访问标志；指示软件是否已访问受此条目控制的 4-KByte 页。如果 EPTP 的第 6 位为 0，则忽略此标志。
		*
		* @see Vol3C[28.2.4(EPT 的已访问和脏位)]
		*/
		UINT64 Accessed : 1;

		/**
		 * [Bit 9] If bit 6 of EPTP is 1, dirty flag for EPT; indicates whether software has written to the 4-KByte page referenced
		 * by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [位 9] 如果 EPTP 的第 6 位为 1，则表示用于 EPT 的脏位；指示软件是否已对受此条目控制的 4-KByte 页进行写入。如果 EPTP 的第 6 位为 0，则忽略此标志。
		*
		* @see Vol3C[28.2.4(EPT 的已访问和脏位)]
		*/
		UINT64 Dirty : 1;

		/**
		 * [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 4-KByte page controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */

		/**
		* [位 10] 用户模式线性地址的执行访问权限。如果 "基于模式的执行控制用于 EPT" VM-execution 控制位为 1，则表示是否允许从受此条目控制的 4-KByte 页中的用户模式线性地址进行指令获取。如果该控制位为 0，则忽略此位。
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved2 : 1;

		/**
		 * [Bits 47:12] Physical address of the 4-KByte page referenced by this entry.
		 */

		/**
		* [位 47:12] 引用此条目的 4-KByte 页的物理地址。
		*/
		UINT64 PageFrameNumber : 36;
		UINT64 Reserved3 : 15;

		/**
		 * [Bit 63] Suppress \#VE. If the "EPT-violation \#VE" VM-execution control is 1, EPT violations caused by accesses to this
		 * page are convertible to virtualization exceptions only if this bit is 0. If "EPT-violation \#VE" VMexecution control is
		 * 0, this bit is ignored.
		 *
		 * @see Vol3C[25.5.6.1(Convertible EPT Violations)]
		 */

		/**
		* [位 63] 抑制 #VE。如果 "EPT-violation #VE" VM-execution 控制位为 1，则仅当此位为 0 时，由于访问此页面而导致的 EPT 违规可以转换为虚拟化异常。如果 "EPT-violation #VE" VM-execution 控制位为 0，则忽略此位。
		*
		* @see Vol3C[25.5.6.1(可转换的 EPT 违规)]
		*/
		UINT64 SuppressVe : 1;
	};

	UINT64 Flags;
} EPTE, * PEPTE;



//////////////////////////////////////////////////
//				      typedefs         			 //
//////////////////////////////////////////////////

typedef EPT_PML4 EPT_PML4_POINTER, * PEPT_PML4_POINTER;
typedef EPDPTE EPT_PML3_POINTER, * PEPT_PML3_POINTER;
typedef EPDE_2MB EPT_PML2_ENTRY, * PEPT_PML2_ENTRY;
typedef EPDE EPT_PML2_POINTER, * PEPT_PML2_POINTER;
typedef EPTE EPT_PML1_ENTRY, * PEPT_PML1_ENTRY;


//////////////////////////////////////////////////
//			     Structs Cont.                	//
//////////////////////////////////////////////////

typedef struct _VMM_EPT_PAGE_TABLE
{
	/**
	 * 28.2.2 Describes 512 contiguous 512GB memory regions each with 512 1GB regions.
	 */

    /**
	28.2.2 描述了512个连续的512GB内存区域，每个区域内又包含512个1GB的区域。
	*/
	DECLSPEC_ALIGN(PAGE_SIZE) EPT_PML4_POINTER PML4[VMM_EPT_PML4E_COUNT];

	/**
	 * Describes exactly 512 contiguous 1GB memory regions within a our singular 512GB PML4 region.
	 */

	/**
	描述了在我们的单个512GB PML4区域中恰好有512个连续的1GB内存区域。
	*/
	DECLSPEC_ALIGN(PAGE_SIZE) EPT_PML3_POINTER PML3[VMM_EPT_PML3E_COUNT];

	/**
	 * For each 1GB PML3 entry, create 512 2MB entries to map identity.
	 * NOTE: We are using 2MB pages as the smallest paging size in our map, so we do not manage individiual 4096 byte pages.
	 * Therefore, we do not allocate any PML1 (4096 byte) paging structures.
	 */

	/**
	对于每个1GB的PML3条目，创建512个2MB的条目来映射为身份映射。
	注意：我们在映射中使用2MB的页面作为最小的分页大小，因此我们不管理单个的4096字节页面。
	因此，我们不分配任何PML1（4096字节）的分页结构。
	*/
	DECLSPEC_ALIGN(PAGE_SIZE) EPT_PML2_ENTRY PML2[VMM_EPT_PML3E_COUNT][VMM_EPT_PML2E_COUNT];


} VMM_EPT_PAGE_TABLE, * PVMM_EPT_PAGE_TABLE;


typedef union _EPTP
{
	struct
	{
		/**
		 * [Bits 2:0] EPT paging-structure memory type:
		 * - 0 = Uncacheable (UC)
		 * - 6 = Write-back (WB)
		 * Other values are reserved.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		[位 2:0] EPT 分页结构内存类型：
		0 = 不可缓存 (UC)
		6 = 写回 (WB)
		其他值为保留值。
		参见 Vol3C[28.2.6(EPT and memory Typing)]。
		*/
		UINT64 MemoryType : 3;

		/**
		 * [Bits 5:3] This value is 1 less than the EPT page-walk length.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		[位 5:3] 这个值比 EPT 页面步进长度小 1。
		参见 Vol3C[28.2.6(EPT and memory Typing)]。
		*/
		UINT64 PageWalkLength : 3;

		/**
		 * [Bit 6] Setting this control to 1 enables accessed and dirty flags for EPT.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		[位 6] 将此控制设置为 1 可以启用 EPT 的访问和脏标志。
		参见 Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]。
		*/
		UINT64 EnableAccessAndDirtyFlags : 1;
		UINT64 Reserved1 : 5;

		/**
		 * [Bits 47:12] Bits N-1:12 of the physical address of the 4-KByte aligned EPT PML4 table.
		 */

		/**
		[位 47:12] 物理地址中对应于4KB对齐的EPT PML4表的位 N-1:12。
		*/
		UINT64 PageFrameNumber : 36;
		UINT64 Reserved2 : 16;
	};

	UINT64 Flags;
} EPTP, * PEPTP;


// MSR_IA32_MTRR_DEF_TYPE 
typedef union _IA32_MTRR_DEF_TYPE_REGISTER
{
	struct
	{
		/**
		 * [Bits 2:0] Default Memory Type.
		 */

		/**
		[位 2:0] 默认内存类型。
		*/
		UINT64 DefaultMemoryType : 3;
		UINT64 Reserved1 : 7;

		/**
		 * [Bit 10] Fixed Range MTRR Enable.
		 */

		/**
		[位 10] 固定范围MTRR使能。
		*/
		UINT64 FixedRangeMtrrEnable : 1;

		/**
		 * [Bit 11] MTRR Enable.
		 */

		/**
		[位 11] MTRR使能。
		*/
		UINT64 MtrrEnable : 1;
		UINT64 Reserved2 : 52;
	};

	UINT64 Flags;
} IA32_MTRR_DEF_TYPE_REGISTER, * PIA32_MTRR_DEF_TYPE_REGISTER;



// MSR_IA32_MTRR_CAPABILITIES
typedef union _IA32_MTRR_CAPABILITIES_REGISTER
{
	struct
	{
		/**
		 * @brief VCNT (variable range registers count) field
		 *
		 * [Bits 7:0] Indicates the number of variable ranges implemented on the processor.
		 */

		/**
		@brief VCNT（可变范围寄存器计数）字段
		[位 7:0] 表示处理器上实现的可变范围的数量。
		*/
		UINT64 VariableRangeCount : 8;

		/**
		 * @brief FIX (fixed range registers supported) flag
		 *
		 * [Bit 8] Fixed range MTRRs (MSR_IA32_MTRR_FIX64K_00000 through MSR_IA32_MTRR_FIX4K_0F8000) are supported when set; no fixed range
		 * registers are supported when clear.
		 */

		/**
		@brief FIX（支持的固定范围寄存器）标志
		[位 8] 当设置时，支持固定范围的MTRRs（从MSR_IA32_MTRR_FIX64K_00000到MSR_IA32_MTRR_FIX4K_0F8000）；当清除时，不支持固定范围的寄存器。
		*/
		UINT64 FixedRangeSupported : 1;
		UINT64 Reserved1 : 1;

		/**
		 * @brief WC (write combining) flag
		 *
		 * [Bit 10] The write-combining (WC) memory type is supported when set; the WC type is not supported when clear.
		 */

		/**
		@brief WC（写合并）标志
		[位 10] 当设置时，支持写合并（WC）内存类型；当清除时，不支持WC类型。
		*/
		UINT64 WcSupported : 1;

		/**
		 * @brief SMRR (System-Management Range Register) flag
		 *
		 * [Bit 11] The system-management range register (SMRR) interface is supported when bit 11 is set; the SMRR interface is
		 * not supported when clear.
		 */

		/**
		@brief SMRR（系统管理范围寄存器）标志
		[位 11] 当设置时，支持系统管理范围寄存器（SMRR）接口；当清除时，不支持SMRR接口。
		*/
		UINT64 SmrrSupported : 1;
		UINT64 Reserved2 : 52;
	};

	UINT64 Flags;
} IA32_MTRR_CAPABILITIES_REGISTER, * PIA32_MTRR_CAPABILITIES_REGISTER;



// MSR_IA32_MTRR_PHYSBASE(0-9)
typedef union _IA32_MTRR_PHYSBASE_REGISTER
{
	struct
	{
		/**
		 * [Bits 7:0] Specifies the memory type for the range.
		 */

        /**
		[位 7:0] 指定范围的内存类型。
		*/
		UINT64 Type : 8;
		UINT64 Reserved1 : 4;

		/**
		 * [Bits 47:12] Specifies the base address of the address range. This 24-bit value, in the case where MAXPHYADDR is 36
		 * bits, is extended by 12 bits at the low end to form the base address (this automatically aligns the address on a 4-KByte
		 * boundary).
		 */

		/**
		[位 47:12] 指定地址范围的基地址。在 MAXPHYADDR 为 36 位的情况下，这个 24 位的值会在低端扩展 12 位来形成基地址（这自动将地址对齐到4KB边界）。
		*/
		UINT64 PageFrameNumber : 36;
		UINT64 Reserved2 : 16;
	};

	UINT64 Flags;
} IA32_MTRR_PHYSBASE_REGISTER, * PIA32_MTRR_PHYSBASE_REGISTER;


// MSR_IA32_MTRR_PHYSMASK(0-9).
typedef union _IA32_MTRR_PHYSMASK_REGISTER
{
	struct
	{
		/**
		 * [Bits 7:0] Specifies the memory type for the range.
		 */

		/**
		[位 7:0] 指定范围的内存类型。
		*/
		UINT64 Type : 8;
		UINT64 Reserved1 : 3;

		/**
		 * [Bit 11] Enables the register pair when set; disables register pair when clear.
		 */

		/**
		[位 11] 当设置时，启用寄存器对；当清除时，禁用寄存器对。
		*/
		UINT64 Valid : 1;

		/**
		 * [Bits 47:12] Specifies a mask (24 bits if the maximum physical address size is 36 bits, 28 bits if the maximum physical
		 * address size is 40 bits). The mask determines the range of the region being mapped, according to the following
		 * relationships:
		 * - Address_Within_Range AND PhysMask = PhysBase AND PhysMask
		 * - This value is extended by 12 bits at the low end to form the mask value.
		 * - The width of the PhysMask field depends on the maximum physical address size supported by the processor.
		 * CPUID.80000008H reports the maximum physical address size supported by the processor. If CPUID.80000008H is not
		 * available, software may assume that the processor supports a 36-bit physical address size.
		 *
		 * @see Vol3A[11.11.3(Example Base and Mask Calculations)]
		 */

		/**
		[位 47:12] 指定一个掩码（如果最大物理地址大小为 36 位，则为 24 位；如果最大物理地址大小为 40 位，则为 28 位）。该掩码根据以下关系确定正在映射的区域范围：
		Address_Within_Range AND PhysMask = PhysBase AND PhysMask
		该值在低端扩展 12 位，形成掩码值。
		PhysMask 字段的宽度取决于处理器支持的最大物理地址大小。CPUID.80000008H 报告处理器支持的最大物理地址大小。如果 CPUID.80000008H 不可用，软件可以假设处理器支持 36 位物理地址大小。
		参见 Vol3A[11.11.3(Example Base and Mask Calculations)]。
		*/
		UINT64 PageFrameNumber : 36;
		UINT64 Reserved2 : 16;
	};

	UINT64 Flags;
} IA32_MTRR_PHYSMASK_REGISTER, * PIA32_MTRR_PHYSMASK_REGISTER;


typedef struct _INVEPT_DESCRIPTOR
{
	UINT64 EptPointer;
	UINT64 Reserved; // Must be zero.
} INVEPT_DESCRIPTOR, * PINVEPT_DESCRIPTOR;

typedef struct _MTRR_RANGE_DESCRIPTOR
{
	SIZE_T PhysicalBaseAddress;
	SIZE_T PhysicalEndAddress;
	UCHAR MemoryType;
} MTRR_RANGE_DESCRIPTOR, * PMTRR_RANGE_DESCRIPTOR;


typedef struct _EPT_STATE
{
    // 钩住页面的详细信息列表
	LIST_ENTRY HookedPagesList;										// A list of the details about hooked pages
    // BIOS在MTRRs中描述的物理内存范围。用于构建EPT身份映射。
	MTRR_RANGE_DESCRIPTOR MemoryRanges[9];							// Physical memory ranges described by the BIOS in the MTRRs. Used to build the EPT identity mapping.
    // 在MemoryRanges中指定的内存范围的数量。
	ULONG NumberOfEnabledMemoryRanges;								// Number of memory ranges specified in MemoryRanges
    // 扩展页表指针（Extended-Page-Table Pointer）
	EPTP   EptPointer;												// Extended-Page-Table Pointer 
	// 用于EPT操作的页表条目（Page Table Entries）
	PVMM_EPT_PAGE_TABLE EptPageTable;							    // Page table entries for EPT operation

} EPT_STATE, * PEPT_STATE;

typedef struct _VMM_EPT_DYNAMIC_SPLIT
{
	/*
	 * The 4096 byte page table entries that correspond to the split 2MB table entry.
	 */

	/*
	对应于分割的2MB表项的4096字节页表条目。
	*/
	DECLSPEC_ALIGN(PAGE_SIZE) EPT_PML1_ENTRY PML1[VMM_EPT_PML1E_COUNT];

	/*
	 * The pointer to the 2MB entry in the page table which this split is servicing.
	 */

	/*
	指向该分割所服务的页表中的2MB条目的指针。
	*/
	union
	{
		PEPT_PML2_ENTRY Entry;
		PEPT_PML2_POINTER Pointer;
	};

	/*
	 * Linked list entries for each dynamic split
	 */

	/*
	每个动态分割的链表条目
	*/
	LIST_ENTRY DynamicSplitList;

} VMM_EPT_DYNAMIC_SPLIT, * PVMM_EPT_DYNAMIC_SPLIT;


typedef union _VMX_EXIT_QUALIFICATION_EPT_VIOLATION
{
	struct
	{
		/**
		 * [Bit 0] Set if the access causing the EPT violation was a data read.
		 */

		/**
		[位 0] 如果导致EPT违规的访问是数据读取，则设置该位。
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Set if the access causing the EPT violation was a data write.
		 */

		/**
		[位 1] 如果导致EPT违规的访问是数据写入，则设置该位。
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] Set if the access causing the EPT violation was an instruction fetch.
		 */

		/**
		[位 2] 如果导致EPT违规的访问是指令获取，则设置该位。
		*/
		UINT64 ExecuteAccess : 1;

		/**
		 * [Bit 3] The logical-AND of bit 0 in the EPT paging-structure entries used to translate the guest-physical address of the
		 * access causing the EPT violation (indicates whether the guest-physical address was readable).
		 */

		/**
		[位 3] 用于翻译导致EPT违规的访问的客户物理地址的EPT分页结构条目中的位0的逻辑与运算结果（指示客户物理地址是否可读）。
		*/
		UINT64 EptReadable : 1;

		/**
		 * [Bit 4] The logical-AND of bit 1 in the EPT paging-structure entries used to translate the guest-physical address of the
		 * access causing the EPT violation (indicates whether the guest-physical address was writeable).
		 */

		/**
		[位 4] 用于翻译导致EPT违规的访问的客户物理地址的EPT分页结构条目中的位1的逻辑与运算结果（指示客户物理地址是否可写）。
		*/
		UINT64 EptWriteable : 1;

		/**
		 * [Bit 5] The logical-AND of bit 2 in the EPT paging-structure entries used to translate the guest-physical address of the
		 * access causing the EPT violation.
		 * If the "mode-based execute control for EPT" VM-execution control is 0, this indicates whether the guest-physical address
		 * was executable. If that control is 1, this indicates whether the guest-physical address was executable for
		 * supervisor-mode linear addresses.
		 */

		/**
		[位 5] 用于翻译导致EPT违规的访问的客户物理地址的EPT分页结构条目中的位2的逻辑与运算结果。
		如果“EPT的基于模式的执行控制”VM执行控制位为0，则指示客户物理地址是否可执行。
		如果该控制位为1，则指示用于特权模式线性地址的客户物理地址是否可执行。
		*/
		UINT64 EptExecutable : 1;

		/**
		 * [Bit 6] If the "mode-based execute control" VM-execution control is 0, the value of this bit is undefined. If that
		 * control is 1, this bit is the logical-AND of bit 10 in the EPT paging-structures entries used to translate the
		 * guest-physical address of the access causing the EPT violation. In this case, it indicates whether the guest-physical
		 * address was executable for user-mode linear addresses.
		 */

		/**
		[位 6] 如果“基于模式的执行控制”VM执行控制位为0，则该位的值未定义。
		如果该控制位为1，则该位是用于翻译导致EPT违规的访问的EPT分页结构条目中的位10的逻辑与运算结果。
		在这种情况下，它指示用户模式线性地址的客户物理地址是否可执行。
		*/
		UINT64 EptExecutableForUserMode : 1;

		/**
		 * [Bit 7] Set if the guest linear-address field is valid. The guest linear-address field is valid for all EPT violations
		 * except those resulting from an attempt to load the guest PDPTEs as part of the execution of the MOV CR instruction.
		 */

		/**
		[位 7] 如果客户线性地址字段有效，则设置该位。客户线性地址字段对于除了在执行MOV CR指令期间尝试加载客户PDPTEs导致的所有EPT违规之外的所有EPT违规都是有效的。
		*/
		UINT64 ValidGuestLinearAddress : 1;

		/**
		 * [Bit 8] If bit 7 is 1:
		 * - Set if the access causing the EPT violation is to a guest-physical address that is the translation of a linear
		 * address.
		 * - Clear if the access causing the EPT violation is to a paging-structure entry as part of a page walk or the update of
		 * an accessed or dirty bit.
		 * Reserved if bit 7 is 0 (cleared to 0).
		 */

		/**
		[位 8] 如果位 7 为 1：
		如果导致 EPT 违规的访问是针对线性地址的翻译的客户物理地址，则设置该位。
		如果导致 EPT 违规的访问是作为页表遍历的一部分或更新访问或脏位的页表条目，则清除该位。
		如果位 7 为 0，则保留（被清除为 0）。
		*/
		UINT64 CausedByTranslation : 1;

		/**
		 * [Bit 9] This bit is 0 if the linear address is a supervisor-mode linear address and 1 if it is a user-mode linear
		 * address. Otherwise, this bit is undefined.
		 *
		 * @remarks If bit 7 is 1, bit 8 is 1, and the processor supports advanced VM-exit information for EPT violations. (If
		 *          CR0.PG = 0, the translation of every linear address is a user-mode linear address and thus this bit will be 1.)
		 */

		/**
		[位 9] 如果线性地址是特权模式线性地址，则该位为 0；如果是用户模式线性地址，则该位为 1。否则，该位的值未定义。
		@remarks 如果位 7 为 1，位 8 为 1，并且处理器支持EPT违规的高级VM退出信息。（如果 CR0.PG = 0，每个线性地址的翻译都是用户模式线性地址，因此该位将为 1。）
		*/
		UINT64 UserModeLinearAddress : 1;

		/**
		 * [Bit 10] This bit is 0 if paging translates the linear address to a read-only page and 1 if it translates to a
		 * read/write page. Otherwise, this bit is undefined
		 *
		 * @remarks If bit 7 is 1, bit 8 is 1, and the processor supports advanced VM-exit information for EPT violations. (If
		 *          CR0.PG = 0, every linear address is read/write and thus this bit will be 1.)
		 */

		/**
		[位 10] 如果分页将线性地址转换为只读页面，则该位为 0；如果将其转换为读/写页面，则为 1。否则，该位的值未定义。
		@remarks 如果位 7 为 1，位 8 为 1，并且处理器支持EPT违规的高级VM退出信息。（如果 CR0.PG = 0，每个线性地址都是读/写的，因此该位将为 1。）
		*/
		UINT64 ReadableWritablePage : 1;

		/**
		 * [Bit 11] This bit is 0 if paging translates the linear address to an executable page and 1 if it translates to an
		 * execute-disable page. Otherwise, this bit is undefined.
		 *
		 * @remarks If bit 7 is 1, bit 8 is 1, and the processor supports advanced VM-exit information for EPT violations. (If
		 *          CR0.PG = 0, CR4.PAE = 0, or MSR_IA32_EFER.NXE = 0, every linear address is executable and thus this bit will be 0.)
		 */

		/**
		[位 11] 如果分页将线性地址转换为可执行页面，则该位为 0；如果将其转换为禁用执行页面，则为 1。否则，该位的值未定义。
		@remarks 如果位 7 为 1，位 8 为 1，并且处理器支持EPT违规的高级VM退出信息。（如果 CR0.PG = 0、CR4.PAE = 0 或 MSR_IA32_EFER.NXE = 0，则每个线性地址都是可执行的，因此该位将为 0。）
		*/
		UINT64 ExecuteDisablePage : 1;

		/**
		 * [Bit 12] NMI unblocking due to IRET.
		 */

		/**
		[位 12] 由于IRET而解除NMI阻塞。
		*/
		UINT64 NmiUnblocking : 1;
		UINT64 Reserved1 : 51;
	};

	UINT64 Flags;
} VMX_EXIT_QUALIFICATION_EPT_VIOLATION, * PVMX_EXIT_QUALIFICATION_EPT_VIOLATION;

// Structure for each hooked instance
// 每个钩住实例的结构体
typedef struct _EPT_HOOKED_PAGE_DETAIL
{

	DECLSPEC_ALIGN(PAGE_SIZE) CHAR FakePageContents[PAGE_SIZE];

	/**
	 * Linked list entires for each page hook.
	 */

	/**
	每个页面钩住的链表条目。
	*/
	LIST_ENTRY PageHookList;

	/**
	* The virtual address from the caller prespective view (cr3)
	*/

	/**
	调用者视角下的虚拟地址（cr3）
	*/
	UINT64 VirtualAddress;

	/**
	 * The base address of the page. Used to find this structure in the list of page hooks
	 * when a hook is hit.
	 */

	/**
	页面的基地址。在命中钩住时用于在页面钩住列表中找到该结构体。
	*/
	SIZE_T PhysicalBaseAddress;

	/**
	* The base address of the page with fake contents. Used to swap page with fake contents
	* when a hook is hit.
	*/
	SIZE_T PhysicalBaseAddressOfFakePageContents;

	/*
	 * The page entry in the page tables that this page is targetting.
	 */

	/*
	该页面针对的页面表中的页面条目。
	*/
	PEPT_PML1_ENTRY EntryAddress;

	/**
	 * The original page entry. Will be copied back when the hook is removed
	 * from the page.
	 */

	/**
	原始页面条目。在从页面中移除钩住时将被复制回去。
	*/
	EPT_PML1_ENTRY OriginalEntry;

	/**
	 * The original page entry. Will be copied back when the hook is remove from the page.
	 */

	/**
	原始页面条目。在从页面中移除钩住时将被复制回来。
	*/
	EPT_PML1_ENTRY ChangedEntry;

	/**
	* The buffer of the trampoline function which is used in the inline hook.
	*/

	/**
	用于内联钩住的跳板函数的缓冲区。
	*/
	PCHAR Trampoline;

	/**
	 * This field shows whether the hook contains a hidden hook for execution or not
	 */

	/**
	该字段指示钩住是否包含用于执行的隐藏钩住。
	*/
	BOOLEAN IsExecutionHook;

} EPT_HOOKED_PAGE_DETAIL, * PEPT_HOOKED_PAGE_DETAIL;


//////////////////////////////////////////////////
//                    Enums		    			//
//////////////////////////////////////////////////

typedef enum _INVEPT_TYPE
{
	INVEPT_SINGLE_CONTEXT = 0x00000001,
	INVEPT_ALL_CONTEXTS = 0x00000002
}INVEPT_TYPE;


//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

// Check for EPT Features
// 检查EPT功能
BOOLEAN EptCheckFeatures();

// Build MTRR Map
// 构建MTRR映射
BOOLEAN EptBuildMtrrMap();

// Hook in VMX Root Mode (A pre-allocated buffer should be available)
// 在VMX Root模式下进行钩住（应该提供一个预分配的缓冲区）
BOOLEAN EptPerformPageHook(PVOID TargetAddress, PVOID HookFunction, PVOID* OrigFunction, BOOLEAN UnsetRead, BOOLEAN UnsetWrite, BOOLEAN UnsetExecute);

// Hook in VMX Non Root Mode
// 在VMX非根模式下进行钩住
BOOLEAN EptPageHook(PVOID TargetAddress, PVOID HookFunction, PVOID* OrigFunction, BOOLEAN SetHookForRead, BOOLEAN SetHookForWrite, BOOLEAN SetHookForExec);

// Initialize EPT Table based on Processor Index
// 根据处理器索引初始化EPT表
BOOLEAN EptLogicalProcessorInitialize();

// Handle EPT Violation
// 处理EPT违规情况
BOOLEAN EptHandleEptViolation(ULONG ExitQualification, UINT64 GuestPhysicalAddr);

// Get the PML1 Entry of a special address
// 获取特定地址的PML1条目
PEPT_PML1_ENTRY EptGetPml1Entry(PVMM_EPT_PAGE_TABLE EptPageTable, SIZE_T PhysicalAddress);

// Handle vm-exits for Monitor Trap Flag to restore previous state
// 处理监视陷阱标志（Monitor Trap Flag）的VM退出，以恢复先前的状态
VOID EptHandleMonitorTrapFlag(PEPT_HOOKED_PAGE_DETAIL HookedEntry);

// Handle Ept Misconfigurations
// 处理EPT配置错误（Ept Misconfigurations）
VOID EptHandleMisconfiguration(UINT64 GuestAddress);

// This function set the specific PML1 entry in a spinlock protected area then	invalidate the TLB , this function should be called from vmx root-mode
// 此函数在受自旋锁保护的区域中设置特定的PML1条目，然后使TLB失效，该函数应该从vmx根模式中调用
VOID EptSetPML1AndInvalidateTLB(PEPT_PML1_ENTRY EntryAddress, EPT_PML1_ENTRY EntryValue, INVEPT_TYPE InvalidationType);

// Handle hooked pages in Vmx-root mode
// 在VMX根模式下处理钩住的页面
BOOLEAN EptHandleHookedPage(EPT_HOOKED_PAGE_DETAIL* HookedEntryDetails, VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification, SIZE_T PhysicalAddress);

// Remove a special hook from the hooked pages lists
// 从钩住的页面列表中移除特定的钩住
BOOLEAN EptPageUnHookSinglePage(SIZE_T PhysicalAddress);

// Remove all hooks from the hooked pages lists
// 从钩住的页面列表中移除所有的钩住
VOID EptPageUnHookAllPages();