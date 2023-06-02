PUBLIC AsmVmxSaveState
PUBLIC AsmVmxRestoreState

EXTERN VmxVirtualizeCurrentSystem:PROC
EXTERN KeIpiGenericCall:PROC
EXTERN InveptAllContexts:PROC

.code _text

;------------------------------------------------------------------------

AsmVmxSaveState PROC
	pushfq	; save r/eflag

	push rax
	push rcx
	push rdx
	push rbx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	sub rsp, 0100h
	; It a x64 FastCall function so the first parameter should go to rcx
	; 这是一个x64 FastCall函数，因此第一个参数应该放在rcx寄存器中

	mov rcx, rsp

	call VmxVirtualizeCurrentSystem

	int 3	; we should never reach here as we execute vmlaunch in the above function.
			; if rax is FALSE then it's an indication of error
			; 我们不应该到达这里，因为我们在上面的函数中执行了vmlaunch指令。
			; 如果rax为FALSE，则表示出现错误的指示
	jmp AsmVmxRestoreState
		
AsmVmxSaveState ENDP

;------------------------------------------------------------------------

AsmVmxRestoreState PROC
	

	add rsp, 0100h
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbp
	pop rbx
	pop rdx
	pop rcx
	pop rax
	
	popfq	; restore r/eflags

	ret
	
AsmVmxRestoreState ENDP

;------------------------------------------------------------------------

END
