#include "Events.h"
#include "Vmx.h"



// Injects interruption to a guest
// 向虚拟机注入中断
VOID EventInjectInterruption(INTERRUPT_TYPE InterruptionType, EXCEPTION_VECTORS Vector, BOOLEAN DeliverErrorCode, ULONG32 ErrorCode)
{
	INTERRUPT_INFO Inject = { 0 };
	Inject.Valid = TRUE;
	Inject.InterruptType = InterruptionType;
	Inject.Vector = Vector;
	Inject.DeliverCode = DeliverErrorCode;
	__vmx_vmwrite(VM_ENTRY_INTR_INFO, Inject.Flags);

	if (DeliverErrorCode) {
		__vmx_vmwrite(VM_ENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
	}
}

/* Inject #BP to the guest (Event Injection) */
/* 向虚拟机注入#BP中断（事件注入） */
VOID EventInjectBreakpoint()
{
	EventInjectInterruption(INTERRUPT_TYPE_SOFTWARE_EXCEPTION, EXCEPTION_VECTOR_BREAKPOINT, FALSE, 0);
	UINT32 ExitInstrLength;
	__vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstrLength);
	__vmx_vmwrite(VM_ENTRY_INSTRUCTION_LEN, ExitInstrLength);
}


/* Inject #GP to the guest (Event Injection) */
/* 向虚拟机注入#GP中断（事件注入） */
VOID EventInjectGeneralProtection()
{
	EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT, TRUE, 0);
	UINT32 ExitInstrLength;
	__vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstrLength);
	__vmx_vmwrite(VM_ENTRY_INSTRUCTION_LEN, ExitInstrLength);
}


/* Inject #UD to the guest (Invalid Opcode - Undefined Opcode) */
/* 向虚拟机注入#UD中断（无效操作码 - 未定义操作码） */
VOID EventInjectUndefinedOpcode()
{
	EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_UNDEFINED_OPCODE, FALSE, 0);
}
