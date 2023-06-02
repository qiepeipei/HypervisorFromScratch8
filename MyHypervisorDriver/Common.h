
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
/* CR0 �е� Intel CPU ��־ */
#define X86_CR0_PE              0x00000001      /* Enable Protected Mode    (RW) */ /* ���ñ���ģʽ����д�� */
#define X86_CR0_MP              0x00000002      /* Monitor Coprocessor      (RW) */ /* ����Э����������д�� */
#define X86_CR0_EM              0x00000004      /* Require FPU Emulation    (RO) */ /* ��Ҫ FPU ģ�⣨ֻ���� */
#define X86_CR0_TS              0x00000008      /* Task Switched            (RW) */ /* �����л�����д�� */
#define X86_CR0_ET              0x00000010      /* Extension type           (RO) */ /* ��չ���ͣ�ֻ���� */
#define X86_CR0_NE              0x00000020      /* Numeric Error Reporting  (RW) */ /* ��ֵ���󱨸棨��д�� */
#define X86_CR0_WP              0x00010000      /* Supervisor Write Protect (RW) */ /* �����д��������д�� */
#define X86_CR0_AM              0x00040000      /* Alignment Checking       (RW) */ /* �����飨��д�� */
#define X86_CR0_NW              0x20000000      /* Not Write-Through        (RW) */ /* ��дͨ����д�� */
#define X86_CR0_CD              0x40000000      /* Cache Disable            (RW) */ /* ���û��棨��д�� */
#define X86_CR0_PG              0x80000000      /* Paging     /* ��ҳ */


/* Intel CPU features in CR4 */
/* CR4 �е� Intel CPU ���� */
#define X86_CR4_VME		0x0001		/* enable vm86 extensions */ /* ���� vm86 ��չ */
#define X86_CR4_PVI		0x0002		/* virtual interrupts flag enable */ /* ���������жϱ�־ */
#define X86_CR4_TSD		0x0004		/* disable time stamp at ipl 3 */ /* �� IPL 3 ����ʱ��� */
#define X86_CR4_DE		0x0008		/* enable debugging extensions */ /* ���õ�����չ */
#define X86_CR4_PSE		0x0010		/* enable page size extensions */ /* ����ҳ���С��չ */
#define X86_CR4_PAE		0x0020		/* enable physical address extensions */ /* ���������ַ��չ */
#define X86_CR4_MCE		0x0040		/* Machine check enable */ /* ���û������ */
#define X86_CR4_PGE		0x0080		/* enable global pages */ /* ����ȫ��ҳ�� */
#define X86_CR4_PCE		0x0100		/* enable performance counters at ipl 3 */ /* �� IPL 3 �������ܼ����� */
#define X86_CR4_OSFXSR		0x0200  /* enable fast FPU save and restore */ /* ���ÿ��� FPU ����ͻָ� */
#define X86_CR4_OSXMMEXCPT	0x0400  /* enable unmasked SSE exceptions */ /* ����δ���ε� SSE �쳣 */
#define X86_CR4_VMXE		0x2000  /* enable VMX */ /* ���� VMX */


// The Microsoft Hypervisor interface defined constants.
// Microsoft Hypervisor �ӿڶ���ĳ�����
#define CPUID_HV_VENDOR_AND_MAX_FUNCTIONS   0x40000000
#define CPUID_HV_INTERFACE                  0x40000001

// CPUID Features
// CPUID ����
#define CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS       0x00000001

// Hypervisor reserved range for RDMSR and WRMSR
// Hypervisor ������ RDMSR �� WRMSR ��Χ
#define RESERVED_MSR_RANGE_LOW 0x40000000
#define RESERVED_MSR_RANGE_HI  0x400000F0

// Alignment Size
// �����С
#define __CPU_INDEX__   KeGetCurrentProcessorNumberEx(NULL)

// Alignment Size
// �����С
#define ALIGNMENT_PAGE_SIZE   4096

// Maximum x64 Address
// x64 ��ַ�����ֵ
#define MAXIMUM_ADDRESS	0xffffffffffffffff

// Pool tag
// �ر�ǩ
#define POOLTAG 0x48564653 // [H]yper[V]isor [F]rom [S]cratch (HVFS)

// System and User ring definitions
// ϵͳ���û����Ķ���
#define DPL_USER                3
#define DPL_SYSTEM              0

// RPL Mask
// RPL ����
#define RPL_MASK                3

// IOCTL Codes and Its meanings
// IOCTL ���뼰�京��
#define IOCTL_TEST 0x1 // In case of testing  // �ڲ��������
// Device type    
// �豸����
#define SIOCTL_TYPE 40000

// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
// �� 0x800 �� 0xFFF �� IOCTL �������빩�û�ʹ�á�
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
	ULONG64 rsp;                  // 0x20         // rsp is not stored here // rsp ��δ�洢�ڴ˴���
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
		unsigned ID : 1;		// Identification flag // ��ʶ��־
		unsigned VIP : 1;		// Virtual interrupt pending // �����жϹ���
		unsigned VIF : 1;		// Virtual interrupt flag // �����жϱ�־
		unsigned AC : 1;		// Alignment check // ������
		unsigned VM : 1;		// Virtual 8086 mode // ���� 8086 ģʽ
		unsigned RF : 1;		// Resume flag // �ָ���־
		unsigned Reserved2 : 1;
		unsigned NT : 1;		// Nested task flag // Ƕ�������־
		unsigned IOPL : 2;		// I/O privilege level // I/O ��Ȩ����
		unsigned OF : 1;
		unsigned DF : 1;
		unsigned IF : 1;		// Interrupt flag // �жϱ�־
		unsigned TF : 1;		// Task flag // �����־
		unsigned SF : 1;		// Sign flag // ���ű�־
		unsigned ZF : 1;		// Zero flag // ���־
		unsigned Reserved3 : 1;
		unsigned AF : 1;		// Borrow flag // ��λ��־
		unsigned Reserved4 : 1;
		unsigned PF : 1;		// Parity flag // ��ż��־
		unsigned Reserved5 : 1;
		unsigned CF : 1;		// Carry flag [Bit 0] // ��λ��־ [λ 0]
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
// ������־����
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
// ���úͻ�ȡ�� MSR λͼ������ص�λ
void SetBit(PVOID Addr, UINT64 bit, BOOLEAN Set);
void GetBit(PVOID Addr, UINT64 bit);

// Run on each logincal Processors functionss
// ��ÿ���߼������������еĺ���
BOOLEAN BroadcastToProcessors(ULONG ProcessorNumber, RunOnLogicalCoreFunc Routine);

// Address Translations
// ��ַת��
UINT64 VirtualAddressToPhysicalAddress(PVOID VirtualAddress);
UINT64 PhysicalAddressToVirtualAddress(UINT64 PhysicalAddress);

// Math :)
// ��ѧ :)
int MathPower(int Base, int Exp);

// Find cr3 of system process
// ����ϵͳ���̵� CR3 ֵ
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
