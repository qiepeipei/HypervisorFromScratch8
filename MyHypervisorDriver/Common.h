
#include "Ept.h"
#include "Configuration.h"
#include "Trace.h"

//////////////////////////////////////////////////
//					Enums						//
//////////////////////////////////////////////////

typedef enum _SEGMENT_REGISTERS
{
	ES = 0,
	CS,
	SS,
	DS,
	FS,
	GS,
	LDTR,
	TR
};


//////////////////////////////////////////////////
//				 Spinlock Funtions				//
//////////////////////////////////////////////////
inline BOOLEAN SpinlockTryLock(LONG* Lock);
inline void SpinlockLock(LONG* Lock);
inline void SpinlockUnlock(LONG* Lock);


//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////

 /* Intel CPU flags in CR0 */
/* CR0 中的 Intel CPU 标志 */
#define X86_CR0_PE              0x00000001      /* Enable Protected Mode    (RW) */ /* 启用保护模式（读写） */
#define X86_CR0_MP              0x00000002      /* Monitor Coprocessor      (RW) */ /* 监视协处理器（读写） */
#define X86_CR0_EM              0x00000004      /* Require FPU Emulation    (RO) */ /* 需要 FPU 模拟（只读） */
#define X86_CR0_TS              0x00000008      /* Task Switched            (RW) */ /* 任务切换（读写） */
#define X86_CR0_ET              0x00000010      /* Extension type           (RO) */ /* 扩展类型（只读） */
#define X86_CR0_NE              0x00000020      /* Numeric Error Reporting  (RW) */ /* 数值错误报告（读写） */
#define X86_CR0_WP              0x00010000      /* Supervisor Write Protect (RW) */ /* 监管者写保护（读写） */
#define X86_CR0_AM              0x00040000      /* Alignment Checking       (RW) */ /* 对齐检查（读写） */
#define X86_CR0_NW              0x20000000      /* Not Write-Through        (RW) */ /* 非写通（读写） */
#define X86_CR0_CD              0x40000000      /* Cache Disable            (RW) */ /* 禁用缓存（读写） */
#define X86_CR0_PG              0x80000000      /* Paging     /* 分页 */


/* Intel CPU features in CR4 */
/* CR4 中的 Intel CPU 功能 */
#define X86_CR4_VME		0x0001		/* enable vm86 extensions */ /* 启用 vm86 扩展 */
#define X86_CR4_PVI		0x0002		/* virtual interrupts flag enable */ /* 启用虚拟中断标志 */
#define X86_CR4_TSD		0x0004		/* disable time stamp at ipl 3 */ /* 在 IPL 3 禁用时间戳 */
#define X86_CR4_DE		0x0008		/* enable debugging extensions */ /* 启用调试扩展 */
#define X86_CR4_PSE		0x0010		/* enable page size extensions */ /* 启用页面大小扩展 */
#define X86_CR4_PAE		0x0020		/* enable physical address extensions */ /* 启用物理地址扩展 */
#define X86_CR4_MCE		0x0040		/* Machine check enable */ /* 启用机器检查 */
#define X86_CR4_PGE		0x0080		/* enable global pages */ /* 启用全局页面 */
#define X86_CR4_PCE		0x0100		/* enable performance counters at ipl 3 */ /* 在 IPL 3 启用性能计数器 */
#define X86_CR4_OSFXSR		0x0200  /* enable fast FPU save and restore */ /* 启用快速 FPU 保存和恢复 */
#define X86_CR4_OSXMMEXCPT	0x0400  /* enable unmasked SSE exceptions */ /* 启用未屏蔽的 SSE 异常 */
#define X86_CR4_VMXE		0x2000  /* enable VMX */ /* 启用 VMX */


// The Microsoft Hypervisor interface defined constants.
// Microsoft Hypervisor 接口定义的常量。
#define CPUID_HV_VENDOR_AND_MAX_FUNCTIONS   0x40000000
#define CPUID_HV_INTERFACE                  0x40000001

// CPUID Features
// CPUID 特性
#define CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS       0x00000001

// Hypervisor reserved range for RDMSR and WRMSR
// Hypervisor 保留的 RDMSR 和 WRMSR 范围
#define RESERVED_MSR_RANGE_LOW 0x40000000
#define RESERVED_MSR_RANGE_HI  0x400000F0

// Alignment Size
// 对齐大小
#define __CPU_INDEX__   KeGetCurrentProcessorNumberEx(NULL)

// Alignment Size
// 对齐大小
#define ALIGNMENT_PAGE_SIZE   4096

// Maximum x64 Address
// x64 地址的最大值
#define MAXIMUM_ADDRESS	0xffffffffffffffff

// Pool tag
// 池标签
#define POOLTAG 0x48564653 // [H]yper[V]isor [F]rom [S]cratch (HVFS)

// System and User ring definitions
// 系统和用户环的定义
#define DPL_USER                3
#define DPL_SYSTEM              0

// RPL Mask
// RPL 掩码
#define RPL_MASK                3

// IOCTL Codes and Its meanings
// IOCTL 代码及其含义
#define IOCTL_TEST 0x1 // In case of testing  // 在测试情况下
// Device type    
// 设备类型
#define SIOCTL_TYPE 40000

// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
// 从 0x800 到 0xFFF 的 IOCTL 函数代码供用户使用。
#define IOCTL_SIOCTL_METHOD_IN_DIRECT \
    CTL_CODE( SIOCTL_TYPE, 0x900, METHOD_IN_DIRECT, FILE_ANY_ACCESS  )

#define IOCTL_SIOCTL_METHOD_OUT_DIRECT \
    CTL_CODE( SIOCTL_TYPE, 0x901, METHOD_OUT_DIRECT , FILE_ANY_ACCESS  )

#define IOCTL_SIOCTL_METHOD_BUFFERED \
    CTL_CODE( SIOCTL_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS  )

#define IOCTL_SIOCTL_METHOD_NEITHER \
    CTL_CODE( SIOCTL_TYPE, 0x903, METHOD_NEITHER , FILE_ANY_ACCESS  )

//////////////////////////////////////////////////
//					 Structures					//
//////////////////////////////////////////////////

typedef struct _GUEST_REGS
{
	ULONG64 rax;                  // 0x00         
	ULONG64 rcx;
	ULONG64 rdx;                  // 0x10
	ULONG64 rbx;
	ULONG64 rsp;                  // 0x20         // rsp is not stored here // rsp 并未存储在此处。
	ULONG64 rbp;
	ULONG64 rsi;                  // 0x30
	ULONG64 rdi;
	ULONG64 r8;                   // 0x40
	ULONG64 r9;
	ULONG64 r10;                  // 0x50
	ULONG64 r11;
	ULONG64 r12;                  // 0x60
	ULONG64 r13;
	ULONG64 r14;                  // 0x70
	ULONG64 r15;
} GUEST_REGS, * PGUEST_REGS;

typedef union _RFLAGS
{
	struct
	{
		unsigned Reserved1 : 10;
		unsigned ID : 1;		// Identification flag // 标识标志
		unsigned VIP : 1;		// Virtual interrupt pending // 虚拟中断挂起
		unsigned VIF : 1;		// Virtual interrupt flag // 虚拟中断标志
		unsigned AC : 1;		// Alignment check // 对齐检查
		unsigned VM : 1;		// Virtual 8086 mode // 虚拟 8086 模式
		unsigned RF : 1;		// Resume flag // 恢复标志
		unsigned Reserved2 : 1;
		unsigned NT : 1;		// Nested task flag // 嵌套任务标志
		unsigned IOPL : 2;		// I/O privilege level // I/O 特权级别
		unsigned OF : 1;
		unsigned DF : 1;
		unsigned IF : 1;		// Interrupt flag // 中断标志
		unsigned TF : 1;		// Task flag // 任务标志
		unsigned SF : 1;		// Sign flag // 符号标志
		unsigned ZF : 1;		// Zero flag // 零标志
		unsigned Reserved3 : 1;
		unsigned AF : 1;		// Borrow flag // 借位标志
		unsigned Reserved4 : 1;
		unsigned PF : 1;		// Parity flag // 奇偶标志
		unsigned Reserved5 : 1;
		unsigned CF : 1;		// Carry flag [Bit 0] // 进位标志 [位 0]
		unsigned Reserved6 : 32;
	};

	ULONG64 Content;
} RFLAGS, * PRFLAGS;

typedef union _SEGMENT_ATTRIBUTES
{
	USHORT UCHARs;
	struct
	{
		USHORT TYPE : 4;              /* 0;  Bit 40-43 */
		USHORT S : 1;                 /* 4;  Bit 44 */
		USHORT DPL : 2;               /* 5;  Bit 45-46 */
		USHORT P : 1;                 /* 7;  Bit 47 */

		USHORT AVL : 1;               /* 8;  Bit 52 */
		USHORT L : 1;                 /* 9;  Bit 53 */
		USHORT DB : 1;                /* 10; Bit 54 */
		USHORT G : 1;                 /* 11; Bit 55 */
		USHORT GAP : 4;

	} Fields;
} SEGMENT_ATTRIBUTES, * PSEGMENT_ATTRIBUTES;

typedef struct _SEGMENT_SELECTOR
{
	USHORT SEL;
	SEGMENT_ATTRIBUTES ATTRIBUTES;
	ULONG32 LIMIT;
	ULONG64 BASE;
} SEGMENT_SELECTOR, * PSEGMENT_SELECTOR;

typedef struct _SEGMENT_DESCRIPTOR
{
	USHORT LIMIT0;
	USHORT BASE0;
	UCHAR  BASE1;
	UCHAR  ATTR0;
	UCHAR  LIMIT1ATTR1;
	UCHAR  BASE2;
} SEGMENT_DESCRIPTOR, * PSEGMENT_DESCRIPTOR;


typedef struct _CPUID
{
	int eax;
	int ebx;
	int ecx;
	int edx;
} CPUID, * PCPUID;

typedef struct _NT_KPROCESS
{
	DISPATCHER_HEADER Header;
	LIST_ENTRY ProfileListHead;
	ULONG_PTR DirectoryTableBase;
	UCHAR Data[1];
}NT_KPROCESS, * PNT_KPROCESS;

//////////////////////////////////////////////////
//				 Function Types					//
//////////////////////////////////////////////////

typedef void(*RunOnLogicalCoreFunc)(ULONG ProcessorID);


//////////////////////////////////////////////////
//					Logging						//
//////////////////////////////////////////////////

// Types
typedef enum _LOG_TYPE
{
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR

}LOG_TYPE;


// Define log variables
// 定义日志变量
#if UseDbgPrintInsteadOfUsermodeMessageTracking
// Use DbgPrint
#define LogInfo(format, ...)  \
    DbgPrint("[+] Information (%s:%d) | " format "\n",	\
		 __func__, __LINE__, __VA_ARGS__)

#define LogWarning(format, ...)  \
    DbgPrint("[-] Warning (%s:%d) | " format "\n",	\
		__func__, __LINE__, __VA_ARGS__)

#define LogError(format, ...)  \
    DbgPrint("[!] Error (%s:%d) | " format "\n",	\
		 __func__, __LINE__, __VA_ARGS__);	\
		DbgBreakPoint()

// Log without any prefix
#define Log(format, ...)  \
    DbgPrint(format "\n", __VA_ARGS__)



#else

#define LogInfo(format, ...)  \
    LogSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE, UseImmediateMessaging, ShowSystemTimeOnDebugMessages, "[+] Information (%s:%d) | " format "\n",	\
		 __func__, __LINE__, __VA_ARGS__)

#define LogInfoImmediate(format, ...)  \
    LogSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE, TRUE, ShowSystemTimeOnDebugMessages, "[+] Information (%s:%d) | " format "\n",	\
		 __func__, __LINE__, __VA_ARGS__)

#define LogWarning(format, ...)  \
    LogSendMessageToQueue(OPERATION_LOG_WARNING_MESSAGE, UseImmediateMessaging, ShowSystemTimeOnDebugMessages, "[-] Warning (%s:%d) | " format "\n",	\
		__func__, __LINE__, __VA_ARGS__)

#define LogError(format, ...)  \
    LogSendMessageToQueue(OPERATION_LOG_ERROR_MESSAGE, UseImmediateMessaging, ShowSystemTimeOnDebugMessages, "[!] Error (%s:%d) | " format "\n",	\
		 __func__, __LINE__, __VA_ARGS__);	\
		DbgBreakPoint()

// Log without any prefix
#define Log(format, ...)  \
    LogSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE, UseImmediateMessaging, ShowSystemTimeOnDebugMessages, format "\n", __VA_ARGS__)


#endif // UseDbgPrintInsteadOfUsermodeMessageTracking

//////////////////////////////////////////////////
//			 Function Definitions				//
//////////////////////////////////////////////////

// Set and Get bits related to MSR Bitmaps Settings
// 设置和获取与 MSR 位图设置相关的位
void SetBit(PVOID Addr, UINT64 bit, BOOLEAN Set);
void GetBit(PVOID Addr, UINT64 bit);

// Run on each logincal Processors functionss
// 在每个逻辑处理器上运行的函数
BOOLEAN BroadcastToProcessors(ULONG ProcessorNumber, RunOnLogicalCoreFunc Routine);

// Address Translations
// 地址转换
UINT64 VirtualAddressToPhysicalAddress(PVOID VirtualAddress);
UINT64 PhysicalAddressToVirtualAddress(UINT64 PhysicalAddress);

// Math :)
// 数学 :)
int MathPower(int Base, int Exp);

// Find cr3 of system process
// 查找系统进程的 CR3 值
UINT64 FindSystemDirectoryTableBase();

//////////////////////////////////////////////////
//			 WDK Major Functions				//
//////////////////////////////////////////////////

// Load & Unload
NTSTATUS DriverEntry(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING  RegistryPath);
VOID DrvUnload(PDRIVER_OBJECT DriverObject);

// IRP Major Functions
NTSTATUS DrvCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DrvRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DrvWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DrvClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DrvUnsupported(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DrvDispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

//////////////////////////////////////////////////
//			  LDE64 (relocatable)				//
//////////////////////////////////////////////////

#define MAX_EXEC_TRAMPOLINE_SIZE	100

extern size_t __fastcall LDE(const void* lpData, unsigned int size);
