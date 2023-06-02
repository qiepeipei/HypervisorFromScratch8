#include "Events.h"
#include "Vmx.h"



// Injects interruption to a guest
// �������ע���ж�
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
/* �������ע��#BP�жϣ��¼�ע�룩 */
VOID EventInjectBreakpoint()
{
	EventInjectInterruption(INTERRUPT_TYPE_SOFTWARE_EXCEPTION, EXCEPTION_VECTOR_BREAKPOINT, FALSE, 0);
	UINT32 ExitInstrLength;
	__vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstrLength);
	__vmx_vmwrite(VM_ENTRY_INSTRUCTION_LEN, ExitInstrLength);
}


/* Inject #GP to the guest (Event Injection) */
/* �������ע��#GP�жϣ��¼�ע�룩 */
VOID EventInjectGeneralProtection()
{
	EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT, TRUE, 0);
	UINT32 ExitInstrLength;
	__vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstrLength);
	__vmx_vmwrite(VM_ENTRY_INSTRUCTION_LEN, ExitInstrLength);
}


/* Inject #UD to the guest (Invalid Opcode - Undefined Opcode) */
/* �������ע��#UD�жϣ���Ч������ - δ��������룩 */
VOID EventInjectUndefinedOpcode()
{
	EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_UNDEFINED_OPCODE, FALSE, 0);
}
