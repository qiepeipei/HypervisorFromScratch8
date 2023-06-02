#pragma once



//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////

// MTRR Physical Base MSRs
// MTRR �����ַ MSR
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
// MTRR �������� MSR
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
// �ڴ�����
#define MEMORY_TYPE_UNCACHEABLE                                      0x00000000
#define MEMORY_TYPE_WRITE_COMBINING                                  0x00000001
#define MEMORY_TYPE_WRITE_THROUGH                                    0x00000004
#define MEMORY_TYPE_WRITE_PROTECTED                                  0x00000005
#define MEMORY_TYPE_WRITE_BACK                                       0x00000006
#define MEMORY_TYPE_INVALID                                          0x000000FF


// Page attributes for internal use
// �����ڲ�ʹ�õ�ҳ������
#define PAGE_ATTRIB_READ											 0x2       
#define PAGE_ATTRIB_WRITE											 0x4       
#define PAGE_ATTRIB_EXEC											 0x8       


// VMX EPT & VPID Capabilities MSR
// VMX EPT �� VPID ���� MSR
#define MSR_IA32_VMX_EPT_VPID_CAP                                        0x0000048C

// MTRR Def MSR
// MTRR Ĭ�� MSR
#define MSR_IA32_MTRR_DEF_TYPE                                           0x000002FF

// MTRR Capabilities MSR
// MTRR ���� MSR
#define MSR_IA32_MTRR_CAPABILITIES                                       0x000000FE

// The number of 512GB PML4 entries in the page table/
// ҳ���� 512GB PML4 ��Ŀ������
#define VMM_EPT_PML4E_COUNT 512

// The number of 1GB PDPT entries in the page table per 512GB PML4 entry.
// ÿ�� 512GB PML4 ��Ŀ��ҳ���� 1GB PDPT ��Ŀ������
#define VMM_EPT_PML3E_COUNT 512

// Then number of 2MB Page Directory entries in the page table per 1GB PML3 entry.
// ÿ�� 1GB PML3 ��Ŀ��ҳ���� 2MB Page Directory ��Ŀ������
#define VMM_EPT_PML2E_COUNT 512

// Then number of 4096 byte Page Table entries in the page table per 2MB PML2 entry when dynamically split.
// ����̬���ʱ��ÿ�� 2MB PML2 ��Ŀ��ҳ���� 4096 �ֽ� Page Table ��Ŀ������
#define VMM_EPT_PML1E_COUNT 512

// Integer 2MB
// ���� 2MB
#define SIZE_2_MB ((SIZE_T)(512 * PAGE_SIZE))

// Offset into the 1st paging structure (4096 byte)
// ƫ���������һ����ҳ�ṹ��4096 �ֽڣ�
#define ADDRMASK_EPT_PML1_OFFSET(_VAR_) (_VAR_ & 0xFFFULL)

// Index of the 1st paging structure (4096 byte)
// ��һ����ҳ�ṹ��4096 �ֽڣ�������
#define ADDRMASK_EPT_PML1_INDEX(_VAR_) ((_VAR_ & 0x1FF000ULL) >> 12)

// Index of the 2nd paging structure (2MB)
// �ڶ�����ҳ�ṹ��2MB��������
#define ADDRMASK_EPT_PML2_INDEX(_VAR_) ((_VAR_ & 0x3FE00000ULL) >> 21)

// Index of the 3rd paging structure (1GB)
// ��������ҳ�ṹ��1GB��������
#define ADDRMASK_EPT_PML3_INDEX(_VAR_) ((_VAR_ & 0x7FC0000000ULL) >> 30)

// Index of the 4th paging structure (512GB)
// ���ĸ���ҳ�ṹ��512GB��������
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
���ڱ��� LIST_ENTRY �ṹ����������ꡣ
LISTHEAD ��ָ������б�ͷ�Ľṹ��ָ�롣
LISTHEAD_NAME �ǰ����б�ͷ�ı��������ơ�Ӧ����ʵ�ʼ�¼���б���ڽṹ��Ա������ƥ�䡣
TARGET_TYPE ���б���ÿ����Ľṹ���������ơ�
TARGET_NAME ��ÿ�ε����н�������ָ��ı��������ơ�
ʾ����
FOR_EACH_LIST_ENTRY(ProcessorContext->EptPageTable, DynamicSplitList, VMM_EPT_DYNAMIC_SPLIT, Split)
 OsFreeNonpagedMemory(Split);
}
ProcessorContext->EptPageTable->DynamicSplitList ���б��ͷ����
VMM_EPT_DYNAMIC_SPLIT ���б���ÿ����Ľṹ��
Split �ǽ�������ָ��ľֲ����������ơ�
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
������Ҫ�� for ѭ���������ж���ֲ���������˸ú��еĴ����Ż��Եû��ҡ�
��ˣ��ú�ֻ�ǽ��� for each �飬�������ƻ�����༭����ͼ������������Ĺ��ܡ�
*/
# define FOR_EACH_LIST_ENTRY_END() }

  //////////////////////////////////////////////////
  //	    			Variables 		    	  //
  //////////////////////////////////////////////////

// Vmx-root lock for changing EPT PML1 Entry and Invalidating TLB
// ���� EPT PML1 ��Ŀ��ʹ TLB ʧЧ�� Vmx-root ��
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
		 * [λ 0] ������Ϊ 1 ʱ��������֧�� EPT ��ִֻ��ת������֧������������� EPT ��ҳ�ṹ��Ŀ������λ 1:0 Ϊ 0����ʾ���������ݷ��ʣ���
		 * ����λ 2 Ϊ 1����ʾ����ָ���ȡ����
		 */
		UINT64 ExecuteOnlyPages : 1;
		UINT64 Reserved1 : 5;

		/**
		 * [Bit 6] Indicates support for a page-walk length of 4.
		 */

        /**
		* [λ 6] ��ʾ֧��ҳ����Ϊ 4��
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
		* [λ 8] ������Ϊ 1 ʱ���߼���������������� EPT ��ҳ�ṹ���ڴ���������Ϊ���ɻ��� (UC)��
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
		 * [λ 14] ������Ϊ 1 ʱ���߼���������������� EPT ��ҳ�ṹ���ڴ���������Ϊд�� (WB)��
		 */
		UINT64 MemoryTypeWriteBack : 1;
		UINT64 Reserved4 : 1;

		/**
		 * [Bit 16] When set to 1, the logical processor allows software to configure a EPT PDE to map a 2-Mbyte page (by setting
		 * bit 7 in the EPT PDE).
		 */

		/**
		* [λ 16] ������Ϊ 1 ʱ���߼������������������ EPT PDE ��ӳ��һ�� 2MB ҳ��ͨ������ EPT PDE �е�λ 7����
		*/
		UINT64 Pde2MbPages : 1;

		/**
		 * [Bit 17] When set to 1, the logical processor allows software to configure a EPT PDPTE to map a 1-Gbyte page (by setting
		 * bit 7 in the EPT PDPTE).
		 */

        /**
		* [λ 17] ������Ϊ 1 ʱ���߼������������������ EPT PDPTE ��ӳ��һ�� 1GB ҳ��ͨ������ EPT PDPTE �е�λ 7����
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
		* [λ 20] �����ȡΪ 1����֧�� INVEPT ָ�
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
		* [λ 21] ������Ϊ 1 ʱ��֧�� EPT �ķ��ʱ�־�����־��
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
		* [λ 22] ������Ϊ 1 ʱ������������ EPT Υ�汨��߼� VM-exit ��Ϣ��������λ��ȡΪ 1 ʱ�Ž��б��档
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
		* [λ 25] ������Ϊ 1 ʱ��֧�ֵ������� INVEPT ���͡�
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
		* [λ 26] ������Ϊ 1 ʱ��֧��ȫ������ INVEPT ���͡�
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
		* [λ 32] ������Ϊ 1 ʱ��֧�� INVVPID ָ�
		*/
		UINT64 Invvpid : 1;
		UINT64 Reserved8 : 7;

		/**
		 * [Bit 40] When set to 1, the individual-address INVVPID type is supported.
		 */

		/**
		* [λ 40] ������Ϊ 1 ʱ��֧�ֵ���ַ INVVPID ���͡�
		*/
		UINT64 InvvpidIndividualAddress : 1;

		/**
		 * [Bit 41] When set to 1, the single-context INVVPID type is supported.
		 */

        /**
		* [λ 41] ������Ϊ 1 ʱ��֧�ֵ������� INVVPID ���͡�
		*/
		UINT64 InvvpidSingleContext : 1;

		/**
		 * [Bit 42] When set to 1, the all-context INVVPID type is supported.
		 */

		/**
		* [λ 42] ������Ϊ 1 ʱ��֧��ȫ������ INVVPID ���͡�
		*/
		UINT64 InvvpidAllContexts : 1;

		/**
		 * [Bit 43] When set to 1, the single-context-retaining-globals INVVPID type is supported.
		 */

		/**
		 * [λ 43] ������Ϊ 1 ʱ��֧�ֱ���ȫ�ֵĵ������� INVVPID ���͡�
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
		* [λ 0] �����ʣ�ָʾ�Ƿ�������ɸ���Ŀ���Ƶ� 512GB ������ж�ȡ��
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 512-GByte region controlled by this entry.
		 */

		/**
		* [λ 1] д���ʣ�ָʾ�Ƿ�������ɸ���Ŀ���Ƶ� 512GB �������д�롣
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 512-GByte region controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 512-GByte region controlled by this entry.
		 */

		/**
		* [λ 2] ��� "mode-based execute control for EPT" VM-execution ����λΪ 0����ʾִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ɸ���Ŀ���Ƶ� 512GB �������ָ���ȡ��
		* ����ÿ���λΪ 1�����ʾ����ģʽ���Ե�ַ��ִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ɸ���Ŀ���Ƶ� 512GB �����еĳ���ģʽ���Ե�ַ����ָ���ȡ��
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
		* [λ 8] ��� EPTP ��λ 6 Ϊ 1�����ʾ EPT �ķ��ʱ�־��ָʾ����Ƿ��ѷ����ɸ���Ŀ���Ƶ� 512GB ������� EPTP ��λ 6 Ϊ 0������Ըñ�־��
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
		* [λ 10] �û�ģʽ���Ե�ַ��ִ�з���Ȩ�ޡ���� "mode-based execute control for EPT" VM-execution ����λΪ 1�����ʾ�Ƿ�������ɸ���Ŀ���Ƶ� 512GB �����е��û�ģʽ���Ե�ַ����ָ���ȡ������ÿ���λΪ 0������Ըñ�־��
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved3 : 1;

		/**
		 * [Bits 47:12] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */

		/**
		* [λ 47:12] ����Ŀ���õİ� 4KB ����� EPT ҳĿ¼ָ���������ַ��
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
		* [λ 0] ������Ȩ�ޣ�ָʾ�Ƿ�������ɸ���Ŀ���õ� 1GB ҳ���ж�ȡ��
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 1-GByte page referenced by this entry.
		 */

        /**
		* [λ 1] д����Ȩ�ޣ�ָʾ�Ƿ�������ɸ���Ŀ���õ� 1GB ҳ����д�롣
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 1-GByte page controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 1-GByte page controlled by this entry.
		 */

		/**
		* [λ 2] ��� "mode-based execute control for EPT" VM-execution ����λΪ 0��ִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ɸ���Ŀ���Ƶ� 1GB ҳ����ָ���ȡ��
		* ����ÿ���λΪ 1�����ʾ����ģʽ���Ե�ַ��ִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ɸ���Ŀ���Ƶ� 1GB ҳ�еĳ���ģʽ���Ե�ַ����ָ���ȡ��
		*/
		UINT64 ExecuteAccess : 1;

		/**
		 * [Bits 5:3] EPT memory type for this 1-GByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

        /**
		* [λ 5:3] �� 1GB ҳ�� EPT �ڴ����͡�
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
		* [λ 6] ���Դ� 1GB ҳ�� PAT �ڴ����͡�
		*
		* @see Vol3C[28.2.6(EPT and memory Typing)]
		*/
		UINT64 IgnorePat : 1;

		/**
		 * [Bit 7] Must be 1 (otherwise, this entry references an EPT page directory).
		 */

		/**
		* [λ 7] ����Ϊ 1�����򣬸���Ŀ���õ��� EPT ҳĿ¼����
		*/
		UINT64 LargePage : 1;

		/**
		 * [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 1-GByte page
		 * referenced by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [λ 8] ��� EPTP ��λ 6 Ϊ 1����Ϊ EPT �ķ��ʱ�־��ָʾ����Ƿ�����˸���Ŀ���õ� 1GB ҳ����� EPTP ��λ 6 Ϊ 0������Դ˱�־��
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
		* [λ 9] ��� EPTP ��λ 6 Ϊ 1����Ϊ EPT ����λ��ָʾ����Ƿ�Ը���Ŀ���õ� 1GB ҳ������д�롣��� EPTP ��λ 6 Ϊ 0������Դ˱�־��
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
		* [λ 10] �û�ģʽ���Ե�ַ��ִ�з���Ȩ�ޡ���� "����ģʽ�� EPT ִ�п���" VM ִ�п���Ϊ 1�����ʾ�Ƿ�����Ӵ���Ŀ�����Ƶ� 1GB ҳ�е��û�ģʽ���Ե�ַ����ָ���ȡ��
		* ����ÿ���Ϊ 0������Դ�λ��
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved1 : 19;

		/**
		 * [Bits 47:30] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */

		/**
		* [λ 47:30] ����Ŀ���õ� 4KB ����� EPT ҳĿ¼ָ���������ַ��
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
		* [λ 63] ���� #VE����� "EPT Υ�� #VE" VM ִ�п���Ϊ 1���������λΪ 0 ʱ���Ը�ҳ�ķ�������� EPT Υ�����ת��Ϊ���⻯�쳣��
		* ��� "EPT Υ�� #VE" VM ִ�п���Ϊ 0������Դ�λ��
		*
		* @see Vol3C[25.5.6.1(��ת���� EPT Υ��)]
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
		* [λ 0] ������Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 1-GByte ������ж�ȡ��
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 1-GByte region controlled by this entry.
		 */

		/**
		* [λ 1] д����Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 1-GByte �������д�롣
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 1-GByte region controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 1-GByte region controlled by this entry.
		 */

		/**
		* [λ 2] ��� "����ģʽ��ִ�п������� EPT" VM-execution ����λΪ 0����ִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 1-GByte �������ָ���ȡ��
		* ����ÿ���λΪ 1�����ʾ�����Ȩ�����Ե�ַ��ִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 1-GByte �����е���Ȩ�����Ե�ַ����ָ���ȡ��
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
		* [λ 8] ��� EPTP �ĵ� 6 λΪ 1�����ʾ���� EPT ���ѷ��ʱ�־��ָʾ����Ƿ��ѷ����ܴ���Ŀ���Ƶ� 1-GByte ������� EPTP �ĵ� 6 λΪ 0������Դ˱�־��
		*
		* @see Vol3C[28.2.4(EPT ���ѷ��ʺ���λ)]
		*/
		UINT64 Accessed : 1;
		UINT64 Reserved2 : 1;

		/**
		 * [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 1-GByte region controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */

		/**
		* [λ 10] �û�ģʽ���Ե�ַ��ִ�з���Ȩ�ޡ���� "����ģʽ��ִ�п������� EPT" VM-execution ����λΪ 1�����ʾ�Ƿ�������ܴ���Ŀ���Ƶ� 1-GByte �����е��û�ģʽ���Ե�ַ����ָ���ȡ������ÿ���λΪ 0������Դ�λ��
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved3 : 1;

		/**
		 * [Bits 47:12] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */

		/**
		* [λ 47:12] ���ô���Ŀ�� 4-KByte ����� EPT ҳĿ¼ָ���������ַ��
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
		* [λ 0] ��ȡ����Ȩ�ޣ�ָʾ�Ƿ�����Ӵ���Ŀ���õ� 2-MByte ҳ���ж�ȡ��
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 2-MByte page referenced by this entry.
		 */

		/**
		* [λ 1] д�����Ȩ�ޣ�ָʾ�Ƿ�����Ӵ���Ŀ���õ� 2-MByte ҳ����д�롣
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 2-MByte page controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 2-MByte page controlled by this entry.
		 */

		/**
		* [λ 2] ��� "����ģʽ��ִ�п������� EPT" VM-execution ����λΪ 0����ִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 2-MByte ҳ����ָ���ȡ��
		* ����ÿ���λΪ 1�����ʾ�����Ȩ�����Ե�ַ��ִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 2-MByte ҳ�е���Ȩ�����Ե�ַ����ָ���ȡ��
		*/
		UINT64 ExecuteAccess : 1;

		/**
		 * [Bits 5:3] EPT memory type for this 2-MByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		 * [λ 5:3] �� 2-MByte ҳ�� EPT �ڴ����͡�
		 *
		 * @see Vol3C[28.2.6(EPT ���ڴ�����)]
		 */
		UINT64 MemoryType : 3;

		/**
		 * [Bit 6] Ignore PAT memory type for this 2-MByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		 * [λ 6] ���Դ� 2-MByte ҳ�� PAT �ڴ����͡�
		 *
		 * @see Vol3C[28.2.6(EPT ���ڴ�����)]
		 */
		UINT64 IgnorePat : 1;

		/**
		 * [Bit 7] Must be 1 (otherwise, this entry references an EPT page table).
		 */

		/**
		* [λ 7] ����Ϊ 1�����򣬴���Ŀ���õ��� EPT ҳ����
		*/
		UINT64 LargePage : 1;

		/**
		 * [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 2-MByte page
		 * referenced by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [λ 8] ��� EPTP �ĵ� 6 λΪ 1�����ʾ���� EPT ���ѷ��ʱ�־��ָʾ����Ƿ��ѷ����ܴ���Ŀ���Ƶ� 2-MByte ҳ����� EPTP �ĵ� 6 λΪ 0������Դ˱�־��
		*
		* @see Vol3C[28.2.4(EPT ���ѷ��ʺ���λ)]
		*/
		UINT64 Accessed : 1;

		/**
		 * [Bit 9] If bit 6 of EPTP is 1, dirty flag for EPT; indicates whether software has written to the 2-MByte page referenced
		 * by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [λ 9] ��� EPTP �ĵ� 6 λΪ 1�����ʾ���� EPT ����λ��ָʾ����Ƿ��Ѷ��ܴ���Ŀ���Ƶ� 2-MByte ҳ����д�롣��� EPTP �ĵ� 6 λΪ 0������Դ˱�־��
		*
		* @see Vol3C[28.2.4(EPT ���ѷ��ʺ���λ)]
		*/
		UINT64 Dirty : 1;

		/**
		 * [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 2-MByte page controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */

		/**
		* [λ 10] �û�ģʽ���Ե�ַ��ִ�з���Ȩ�ޡ���� "����ģʽ��ִ�п������� EPT" VM-execution ����λΪ 1�����ʾ�Ƿ�������ܴ���Ŀ���Ƶ� 2-MByte ҳ�е��û�ģʽ���Ե�ַ����ָ���ȡ������ÿ���λΪ 0������Դ�λ��
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved1 : 10;

		/**
		 * [Bits 47:21] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */

		/**
		* [λ 47:21] ���ô���Ŀ�� 4-KByte ����� EPT ҳĿ¼ָ���������ַ��
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
		* [λ 63] ���� #VE����� "EPT-violation #VE" VM-execution ����λΪ 1���������λΪ 0 ʱ�����ڷ��ʴ�ҳ������µ� EPT Υ�����ת��Ϊ���⻯�쳣����� "EPT-violation #VE" VM-execution ����λΪ 0������Դ�λ��
		*
		* @see Vol3C[25.5.6.1(��ת���� EPT Υ��)]
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
		* [λ 0] ��ȡ����Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 2-MByte ������ж�ȡ��
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 2-MByte region controlled by this entry.
		 */

		/**
		* [λ 1] д�����Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 2-MByte �������д�롣
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 2-MByte region controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 2-MByte region controlled by this entry.
		 */

		/**
		* [λ 2] ��� "����ģʽ��ִ�п������� EPT" VM-execution ����λΪ 0����ִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 2-MByte �������ָ���ȡ��
		* ����ÿ���λΪ 1�����ʾ�����Ȩ�����Ե�ַ��ִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 2-MByte �����е���Ȩ�����Ե�ַ����ָ���ȡ��
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
		* [λ 8] ��� EPTP �ĵ� 6 λΪ 1�����ʾ���� EPT ���ѷ��ʱ�־��ָʾ����Ƿ��ѷ����ܴ���Ŀ���Ƶ� 2-MByte ������� EPTP �ĵ� 6 λΪ 0������Դ˱�־��
		*
		* @see Vol3C[28.2.4(EPT ���ѷ��ʺ���λ)]
		*/
		UINT64 Accessed : 1;
		UINT64 Reserved2 : 1;

		/**
		 * [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 2-MByte region controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */

		/**
		* [λ 10] �û�ģʽ���Ե�ַ��ִ�з���Ȩ�ޡ���� "����ģʽ��ִ�п������� EPT" VM-execution ����λΪ 1�����ʾ�Ƿ�������ܴ���Ŀ���Ƶ� 2-MByte �����е��û�ģʽ���Ե�ַ����ָ���ȡ������ÿ���λΪ 0������Դ�λ��
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved3 : 1;

		/**
		 * [Bits 47:12] Physical address of 4-KByte aligned EPT page table referenced by this entry.
		 */

		/**
		* [λ 47:12] ���ô���Ŀ�� 4-KByte ����� EPT ҳ��������ַ��
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
		* [λ 0] ��ȡ����Ȩ�ޣ�ָʾ�Ƿ�����Ӵ���Ŀ���õ� 4-KByte ҳ���ж�ȡ��
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Write access; indicates whether writes are allowed from the 4-KByte page referenced by this entry.
		 */

		/**
		* [λ 1] д�����Ȩ�ޣ�ָʾ�Ƿ�����Ӵ���Ŀ���õ� 4-KByte ҳ����д�롣
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 4-KByte page controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 4-KByte page controlled by this entry.
		 */

		/**
		* [λ 2] ��� "����ģʽ��ִ�п������� EPT" VM-execution ����λΪ 0����ִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 4-KByte ҳ����ָ���ȡ��
		* ����ÿ���λΪ 1�����ʾ�����Ȩ�����Ե�ַ��ִ�з���Ȩ�ޣ�ָʾ�Ƿ�������ܴ���Ŀ���Ƶ� 4-KByte ҳ�е���Ȩ�����Ե�ַ����ָ���ȡ��
		*/
		UINT64 ExecuteAccess : 1;

		/**
		 * [Bits 5:3] EPT memory type for this 4-KByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		* [λ 5:3] �� 4-KByte ҳ�� EPT �ڴ����͡�
		*
		* @see Vol3C[28.2.6(EPT ���ڴ�����)]
		*/
		UINT64 MemoryType : 3;

		/**
		 * [Bit 6] Ignore PAT memory type for this 4-KByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		* [λ 6] ���Դ� 4-KByte ҳ�� PAT �ڴ����͡�
		*
		* @see Vol3C[28.2.6(EPT ���ڴ�����)]
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
		* [λ 8] ��� EPTP �ĵ� 6 λΪ 1�����ʾ���� EPT ���ѷ��ʱ�־��ָʾ����Ƿ��ѷ����ܴ���Ŀ���Ƶ� 4-KByte ҳ����� EPTP �ĵ� 6 λΪ 0������Դ˱�־��
		*
		* @see Vol3C[28.2.4(EPT ���ѷ��ʺ���λ)]
		*/
		UINT64 Accessed : 1;

		/**
		 * [Bit 9] If bit 6 of EPTP is 1, dirty flag for EPT; indicates whether software has written to the 4-KByte page referenced
		 * by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		* [λ 9] ��� EPTP �ĵ� 6 λΪ 1�����ʾ���� EPT ����λ��ָʾ����Ƿ��Ѷ��ܴ���Ŀ���Ƶ� 4-KByte ҳ����д�롣��� EPTP �ĵ� 6 λΪ 0������Դ˱�־��
		*
		* @see Vol3C[28.2.4(EPT ���ѷ��ʺ���λ)]
		*/
		UINT64 Dirty : 1;

		/**
		 * [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 4-KByte page controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */

		/**
		* [λ 10] �û�ģʽ���Ե�ַ��ִ�з���Ȩ�ޡ���� "����ģʽ��ִ�п������� EPT" VM-execution ����λΪ 1�����ʾ�Ƿ�������ܴ���Ŀ���Ƶ� 4-KByte ҳ�е��û�ģʽ���Ե�ַ����ָ���ȡ������ÿ���λΪ 0������Դ�λ��
		*/
		UINT64 UserModeExecute : 1;
		UINT64 Reserved2 : 1;

		/**
		 * [Bits 47:12] Physical address of the 4-KByte page referenced by this entry.
		 */

		/**
		* [λ 47:12] ���ô���Ŀ�� 4-KByte ҳ�������ַ��
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
		* [λ 63] ���� #VE����� "EPT-violation #VE" VM-execution ����λΪ 1���������λΪ 0 ʱ�����ڷ��ʴ�ҳ������µ� EPT Υ�����ת��Ϊ���⻯�쳣����� "EPT-violation #VE" VM-execution ����λΪ 0������Դ�λ��
		*
		* @see Vol3C[25.5.6.1(��ת���� EPT Υ��)]
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
	28.2.2 ������512��������512GB�ڴ�����ÿ���������ְ���512��1GB������
	*/
	DECLSPEC_ALIGN(PAGE_SIZE) EPT_PML4_POINTER PML4[VMM_EPT_PML4E_COUNT];

	/**
	 * Describes exactly 512 contiguous 1GB memory regions within a our singular 512GB PML4 region.
	 */

	/**
	�����������ǵĵ���512GB PML4������ǡ����512��������1GB�ڴ�����
	*/
	DECLSPEC_ALIGN(PAGE_SIZE) EPT_PML3_POINTER PML3[VMM_EPT_PML3E_COUNT];

	/**
	 * For each 1GB PML3 entry, create 512 2MB entries to map identity.
	 * NOTE: We are using 2MB pages as the smallest paging size in our map, so we do not manage individiual 4096 byte pages.
	 * Therefore, we do not allocate any PML1 (4096 byte) paging structures.
	 */

	/**
	����ÿ��1GB��PML3��Ŀ������512��2MB����Ŀ��ӳ��Ϊ���ӳ�䡣
	ע�⣺������ӳ����ʹ��2MB��ҳ����Ϊ��С�ķ�ҳ��С��������ǲ���������4096�ֽ�ҳ�档
	��ˣ����ǲ������κ�PML1��4096�ֽڣ��ķ�ҳ�ṹ��
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
		[λ 2:0] EPT ��ҳ�ṹ�ڴ����ͣ�
		0 = ���ɻ��� (UC)
		6 = д�� (WB)
		����ֵΪ����ֵ��
		�μ� Vol3C[28.2.6(EPT and memory Typing)]��
		*/
		UINT64 MemoryType : 3;

		/**
		 * [Bits 5:3] This value is 1 less than the EPT page-walk length.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */

		/**
		[λ 5:3] ���ֵ�� EPT ҳ�沽������С 1��
		�μ� Vol3C[28.2.6(EPT and memory Typing)]��
		*/
		UINT64 PageWalkLength : 3;

		/**
		 * [Bit 6] Setting this control to 1 enables accessed and dirty flags for EPT.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */

		/**
		[λ 6] ���˿�������Ϊ 1 �������� EPT �ķ��ʺ����־��
		�μ� Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]��
		*/
		UINT64 EnableAccessAndDirtyFlags : 1;
		UINT64 Reserved1 : 5;

		/**
		 * [Bits 47:12] Bits N-1:12 of the physical address of the 4-KByte aligned EPT PML4 table.
		 */

		/**
		[λ 47:12] �����ַ�ж�Ӧ��4KB�����EPT PML4���λ N-1:12��
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
		[λ 2:0] Ĭ���ڴ����͡�
		*/
		UINT64 DefaultMemoryType : 3;
		UINT64 Reserved1 : 7;

		/**
		 * [Bit 10] Fixed Range MTRR Enable.
		 */

		/**
		[λ 10] �̶���ΧMTRRʹ�ܡ�
		*/
		UINT64 FixedRangeMtrrEnable : 1;

		/**
		 * [Bit 11] MTRR Enable.
		 */

		/**
		[λ 11] MTRRʹ�ܡ�
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
		@brief VCNT���ɱ䷶Χ�Ĵ����������ֶ�
		[λ 7:0] ��ʾ��������ʵ�ֵĿɱ䷶Χ��������
		*/
		UINT64 VariableRangeCount : 8;

		/**
		 * @brief FIX (fixed range registers supported) flag
		 *
		 * [Bit 8] Fixed range MTRRs (MSR_IA32_MTRR_FIX64K_00000 through MSR_IA32_MTRR_FIX4K_0F8000) are supported when set; no fixed range
		 * registers are supported when clear.
		 */

		/**
		@brief FIX��֧�ֵĹ̶���Χ�Ĵ�������־
		[λ 8] ������ʱ��֧�̶ֹ���Χ��MTRRs����MSR_IA32_MTRR_FIX64K_00000��MSR_IA32_MTRR_FIX4K_0F8000���������ʱ����֧�̶ֹ���Χ�ļĴ�����
		*/
		UINT64 FixedRangeSupported : 1;
		UINT64 Reserved1 : 1;

		/**
		 * @brief WC (write combining) flag
		 *
		 * [Bit 10] The write-combining (WC) memory type is supported when set; the WC type is not supported when clear.
		 */

		/**
		@brief WC��д�ϲ�����־
		[λ 10] ������ʱ��֧��д�ϲ���WC���ڴ����ͣ������ʱ����֧��WC���͡�
		*/
		UINT64 WcSupported : 1;

		/**
		 * @brief SMRR (System-Management Range Register) flag
		 *
		 * [Bit 11] The system-management range register (SMRR) interface is supported when bit 11 is set; the SMRR interface is
		 * not supported when clear.
		 */

		/**
		@brief SMRR��ϵͳ����Χ�Ĵ�������־
		[λ 11] ������ʱ��֧��ϵͳ����Χ�Ĵ�����SMRR���ӿڣ������ʱ����֧��SMRR�ӿڡ�
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
		[λ 7:0] ָ����Χ���ڴ����͡�
		*/
		UINT64 Type : 8;
		UINT64 Reserved1 : 4;

		/**
		 * [Bits 47:12] Specifies the base address of the address range. This 24-bit value, in the case where MAXPHYADDR is 36
		 * bits, is extended by 12 bits at the low end to form the base address (this automatically aligns the address on a 4-KByte
		 * boundary).
		 */

		/**
		[λ 47:12] ָ����ַ��Χ�Ļ���ַ���� MAXPHYADDR Ϊ 36 λ������£���� 24 λ��ֵ���ڵͶ���չ 12 λ���γɻ���ַ�����Զ�����ַ���뵽4KB�߽磩��
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
		[λ 7:0] ָ����Χ���ڴ����͡�
		*/
		UINT64 Type : 8;
		UINT64 Reserved1 : 3;

		/**
		 * [Bit 11] Enables the register pair when set; disables register pair when clear.
		 */

		/**
		[λ 11] ������ʱ�����üĴ����ԣ������ʱ�����üĴ����ԡ�
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
		[λ 47:12] ָ��һ�����루�����������ַ��СΪ 36 λ����Ϊ 24 λ�������������ַ��СΪ 40 λ����Ϊ 28 λ����������������¹�ϵȷ������ӳ�������Χ��
		Address_Within_Range AND PhysMask = PhysBase AND PhysMask
		��ֵ�ڵͶ���չ 12 λ���γ�����ֵ��
		PhysMask �ֶεĿ��ȡ���ڴ�����֧�ֵ���������ַ��С��CPUID.80000008H ���洦����֧�ֵ���������ַ��С����� CPUID.80000008H �����ã�������Լ��账����֧�� 36 λ�����ַ��С��
		�μ� Vol3A[11.11.3(Example Base and Mask Calculations)]��
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
    // ��סҳ�����ϸ��Ϣ�б�
	LIST_ENTRY HookedPagesList;										// A list of the details about hooked pages
    // BIOS��MTRRs�������������ڴ淶Χ�����ڹ���EPT���ӳ�䡣
	MTRR_RANGE_DESCRIPTOR MemoryRanges[9];							// Physical memory ranges described by the BIOS in the MTRRs. Used to build the EPT identity mapping.
    // ��MemoryRanges��ָ�����ڴ淶Χ��������
	ULONG NumberOfEnabledMemoryRanges;								// Number of memory ranges specified in MemoryRanges
    // ��չҳ��ָ�루Extended-Page-Table Pointer��
	EPTP   EptPointer;												// Extended-Page-Table Pointer 
	// ����EPT������ҳ����Ŀ��Page Table Entries��
	PVMM_EPT_PAGE_TABLE EptPageTable;							    // Page table entries for EPT operation

} EPT_STATE, * PEPT_STATE;

typedef struct _VMM_EPT_DYNAMIC_SPLIT
{
	/*
	 * The 4096 byte page table entries that correspond to the split 2MB table entry.
	 */

	/*
	��Ӧ�ڷָ��2MB�����4096�ֽ�ҳ����Ŀ��
	*/
	DECLSPEC_ALIGN(PAGE_SIZE) EPT_PML1_ENTRY PML1[VMM_EPT_PML1E_COUNT];

	/*
	 * The pointer to the 2MB entry in the page table which this split is servicing.
	 */

	/*
	ָ��÷ָ��������ҳ���е�2MB��Ŀ��ָ�롣
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
	ÿ����̬�ָ��������Ŀ
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
		[λ 0] �������EPTΥ��ķ��������ݶ�ȡ�������ø�λ��
		*/
		UINT64 ReadAccess : 1;

		/**
		 * [Bit 1] Set if the access causing the EPT violation was a data write.
		 */

		/**
		[λ 1] �������EPTΥ��ķ���������д�룬�����ø�λ��
		*/
		UINT64 WriteAccess : 1;

		/**
		 * [Bit 2] Set if the access causing the EPT violation was an instruction fetch.
		 */

		/**
		[λ 2] �������EPTΥ��ķ�����ָ���ȡ�������ø�λ��
		*/
		UINT64 ExecuteAccess : 1;

		/**
		 * [Bit 3] The logical-AND of bit 0 in the EPT paging-structure entries used to translate the guest-physical address of the
		 * access causing the EPT violation (indicates whether the guest-physical address was readable).
		 */

		/**
		[λ 3] ���ڷ��뵼��EPTΥ��ķ��ʵĿͻ������ַ��EPT��ҳ�ṹ��Ŀ�е�λ0���߼�����������ָʾ�ͻ������ַ�Ƿ�ɶ�����
		*/
		UINT64 EptReadable : 1;

		/**
		 * [Bit 4] The logical-AND of bit 1 in the EPT paging-structure entries used to translate the guest-physical address of the
		 * access causing the EPT violation (indicates whether the guest-physical address was writeable).
		 */

		/**
		[λ 4] ���ڷ��뵼��EPTΥ��ķ��ʵĿͻ������ַ��EPT��ҳ�ṹ��Ŀ�е�λ1���߼�����������ָʾ�ͻ������ַ�Ƿ��д����
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
		[λ 5] ���ڷ��뵼��EPTΥ��ķ��ʵĿͻ������ַ��EPT��ҳ�ṹ��Ŀ�е�λ2���߼�����������
		�����EPT�Ļ���ģʽ��ִ�п��ơ�VMִ�п���λΪ0����ָʾ�ͻ������ַ�Ƿ��ִ�С�
		����ÿ���λΪ1����ָʾ������Ȩģʽ���Ե�ַ�Ŀͻ������ַ�Ƿ��ִ�С�
		*/
		UINT64 EptExecutable : 1;

		/**
		 * [Bit 6] If the "mode-based execute control" VM-execution control is 0, the value of this bit is undefined. If that
		 * control is 1, this bit is the logical-AND of bit 10 in the EPT paging-structures entries used to translate the
		 * guest-physical address of the access causing the EPT violation. In this case, it indicates whether the guest-physical
		 * address was executable for user-mode linear addresses.
		 */

		/**
		[λ 6] ���������ģʽ��ִ�п��ơ�VMִ�п���λΪ0�����λ��ֵδ���塣
		����ÿ���λΪ1�����λ�����ڷ��뵼��EPTΥ��ķ��ʵ�EPT��ҳ�ṹ��Ŀ�е�λ10���߼�����������
		����������£���ָʾ�û�ģʽ���Ե�ַ�Ŀͻ������ַ�Ƿ��ִ�С�
		*/
		UINT64 EptExecutableForUserMode : 1;

		/**
		 * [Bit 7] Set if the guest linear-address field is valid. The guest linear-address field is valid for all EPT violations
		 * except those resulting from an attempt to load the guest PDPTEs as part of the execution of the MOV CR instruction.
		 */

		/**
		[λ 7] ����ͻ����Ե�ַ�ֶ���Ч�������ø�λ���ͻ����Ե�ַ�ֶζ��ڳ�����ִ��MOV CRָ���ڼ䳢�Լ��ؿͻ�PDPTEs���µ�����EPTΥ��֮�������EPTΥ�涼����Ч�ġ�
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
		[λ 8] ���λ 7 Ϊ 1��
		������� EPT Υ��ķ�����������Ե�ַ�ķ���Ŀͻ������ַ�������ø�λ��
		������� EPT Υ��ķ�������Ϊҳ�������һ���ֻ���·��ʻ���λ��ҳ����Ŀ���������λ��
		���λ 7 Ϊ 0�������������Ϊ 0����
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
		[λ 9] ������Ե�ַ����Ȩģʽ���Ե�ַ�����λΪ 0��������û�ģʽ���Ե�ַ�����λΪ 1�����򣬸�λ��ֵδ���塣
		@remarks ���λ 7 Ϊ 1��λ 8 Ϊ 1�����Ҵ�����֧��EPTΥ��ĸ߼�VM�˳���Ϣ������� CR0.PG = 0��ÿ�����Ե�ַ�ķ��붼���û�ģʽ���Ե�ַ����˸�λ��Ϊ 1����
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
		[λ 10] �����ҳ�����Ե�ַת��Ϊֻ��ҳ�棬���λΪ 0���������ת��Ϊ��/дҳ�棬��Ϊ 1�����򣬸�λ��ֵδ���塣
		@remarks ���λ 7 Ϊ 1��λ 8 Ϊ 1�����Ҵ�����֧��EPTΥ��ĸ߼�VM�˳���Ϣ������� CR0.PG = 0��ÿ�����Ե�ַ���Ƕ�/д�ģ���˸�λ��Ϊ 1����
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
		[λ 11] �����ҳ�����Ե�ַת��Ϊ��ִ��ҳ�棬���λΪ 0���������ת��Ϊ����ִ��ҳ�棬��Ϊ 1�����򣬸�λ��ֵδ���塣
		@remarks ���λ 7 Ϊ 1��λ 8 Ϊ 1�����Ҵ�����֧��EPTΥ��ĸ߼�VM�˳���Ϣ������� CR0.PG = 0��CR4.PAE = 0 �� MSR_IA32_EFER.NXE = 0����ÿ�����Ե�ַ���ǿ�ִ�еģ���˸�λ��Ϊ 0����
		*/
		UINT64 ExecuteDisablePage : 1;

		/**
		 * [Bit 12] NMI unblocking due to IRET.
		 */

		/**
		[λ 12] ����IRET�����NMI������
		*/
		UINT64 NmiUnblocking : 1;
		UINT64 Reserved1 : 51;
	};

	UINT64 Flags;
} VMX_EXIT_QUALIFICATION_EPT_VIOLATION, * PVMX_EXIT_QUALIFICATION_EPT_VIOLATION;

// Structure for each hooked instance
// ÿ����סʵ���Ľṹ��
typedef struct _EPT_HOOKED_PAGE_DETAIL
{

	DECLSPEC_ALIGN(PAGE_SIZE) CHAR FakePageContents[PAGE_SIZE];

	/**
	 * Linked list entires for each page hook.
	 */

	/**
	ÿ��ҳ�湳ס��������Ŀ��
	*/
	LIST_ENTRY PageHookList;

	/**
	* The virtual address from the caller prespective view (cr3)
	*/

	/**
	�������ӽ��µ������ַ��cr3��
	*/
	UINT64 VirtualAddress;

	/**
	 * The base address of the page. Used to find this structure in the list of page hooks
	 * when a hook is hit.
	 */

	/**
	ҳ��Ļ���ַ�������й�סʱ������ҳ�湳ס�б����ҵ��ýṹ�塣
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
	��ҳ����Ե�ҳ����е�ҳ����Ŀ��
	*/
	PEPT_PML1_ENTRY EntryAddress;

	/**
	 * The original page entry. Will be copied back when the hook is removed
	 * from the page.
	 */

	/**
	ԭʼҳ����Ŀ���ڴ�ҳ�����Ƴ���סʱ�������ƻ�ȥ��
	*/
	EPT_PML1_ENTRY OriginalEntry;

	/**
	 * The original page entry. Will be copied back when the hook is remove from the page.
	 */

	/**
	ԭʼҳ����Ŀ���ڴ�ҳ�����Ƴ���סʱ�������ƻ�����
	*/
	EPT_PML1_ENTRY ChangedEntry;

	/**
	* The buffer of the trampoline function which is used in the inline hook.
	*/

	/**
	����������ס�����庯���Ļ�������
	*/
	PCHAR Trampoline;

	/**
	 * This field shows whether the hook contains a hidden hook for execution or not
	 */

	/**
	���ֶ�ָʾ��ס�Ƿ��������ִ�е����ع�ס��
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
// ���EPT����
BOOLEAN EptCheckFeatures();

// Build MTRR Map
// ����MTRRӳ��
BOOLEAN EptBuildMtrrMap();

// Hook in VMX Root Mode (A pre-allocated buffer should be available)
// ��VMX Rootģʽ�½��й�ס��Ӧ���ṩһ��Ԥ����Ļ�������
BOOLEAN EptPerformPageHook(PVOID TargetAddress, PVOID HookFunction, PVOID* OrigFunction, BOOLEAN UnsetRead, BOOLEAN UnsetWrite, BOOLEAN UnsetExecute);

// Hook in VMX Non Root Mode
// ��VMX�Ǹ�ģʽ�½��й�ס
BOOLEAN EptPageHook(PVOID TargetAddress, PVOID HookFunction, PVOID* OrigFunction, BOOLEAN SetHookForRead, BOOLEAN SetHookForWrite, BOOLEAN SetHookForExec);

// Initialize EPT Table based on Processor Index
// ���ݴ�����������ʼ��EPT��
BOOLEAN EptLogicalProcessorInitialize();

// Handle EPT Violation
// ����EPTΥ�����
BOOLEAN EptHandleEptViolation(ULONG ExitQualification, UINT64 GuestPhysicalAddr);

// Get the PML1 Entry of a special address
// ��ȡ�ض���ַ��PML1��Ŀ
PEPT_PML1_ENTRY EptGetPml1Entry(PVMM_EPT_PAGE_TABLE EptPageTable, SIZE_T PhysicalAddress);

// Handle vm-exits for Monitor Trap Flag to restore previous state
// ������������־��Monitor Trap Flag����VM�˳����Իָ���ǰ��״̬
VOID EptHandleMonitorTrapFlag(PEPT_HOOKED_PAGE_DETAIL HookedEntry);

// Handle Ept Misconfigurations
// ����EPT���ô���Ept Misconfigurations��
VOID EptHandleMisconfiguration(UINT64 GuestAddress);

// This function set the specific PML1 entry in a spinlock protected area then	invalidate the TLB , this function should be called from vmx root-mode
// �˺������������������������������ض���PML1��Ŀ��Ȼ��ʹTLBʧЧ���ú���Ӧ�ô�vmx��ģʽ�е���
VOID EptSetPML1AndInvalidateTLB(PEPT_PML1_ENTRY EntryAddress, EPT_PML1_ENTRY EntryValue, INVEPT_TYPE InvalidationType);

// Handle hooked pages in Vmx-root mode
// ��VMX��ģʽ�´���ס��ҳ��
BOOLEAN EptHandleHookedPage(EPT_HOOKED_PAGE_DETAIL* HookedEntryDetails, VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification, SIZE_T PhysicalAddress);

// Remove a special hook from the hooked pages lists
// �ӹ�ס��ҳ���б����Ƴ��ض��Ĺ�ס
BOOLEAN EptPageUnHookSinglePage(SIZE_T PhysicalAddress);

// Remove all hooks from the hooked pages lists
// �ӹ�ס��ҳ���б����Ƴ����еĹ�ס
VOID EptPageUnHookAllPages();