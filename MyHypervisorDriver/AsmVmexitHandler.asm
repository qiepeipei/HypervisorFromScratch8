PUBLIC AsmVmexitHandler

EXTERN VmxVmexitHandler:PROC
EXTERN VmxVmresume:PROC
EXTERN HvReturnStackPointerForVmxoff:PROC
EXTERN HvReturnInstructionPointerForVmxoff:PROC


.code _text

;------------------------------------------------------------------------

AsmVmexitHandler PROC
    
    push 0  ; we might be in an unaligned stack state, so the memory before stack might cause 
            ; irql less or equal as it doesn't exist, so we just put some extra space avoid
            ; these kind of erros
            ; 我们可能处于非对齐的堆栈状态，因此堆栈之前的内存可能会引发IRQL小于或等于错误，因此我们留出一些额外空间来避免此类错误

    pushfq

    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8        
    push rdi
    push rsi
    push rbp
    push rbp	; rsp
    push rbx
    push rdx
    push rcx
    push rax	

	mov rcx, rsp		; Fast call argument to PGUEST_REGS ; PGUEST_REGS的快速调用参数
	sub	rsp, 28h		; Free some space for Shadow Section ; 为Shadow Section腾出一些空间
	call	VmxVmexitHandler
	add	rsp, 28h		; Restore the state ; 恢复状态

	cmp	al, 1	; Check whether we have to turn off VMX or Not (the result is in RAX) ; 检查是否需要关闭VMX（结果存储在RAX中）
	je		AsmVmxoffHandler

	RestoreState:
	pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp		; rsp
    pop rbp
    pop rsi
    pop rdi 
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    popfq

	sub rsp, 0100h      ; to avoid error in future functions ; 为了避免将来函数中的错误
	jmp VmxVmresume

AsmVmexitHandler ENDP

;------------------------------------------------------------------------

AsmVmxoffHandler PROC
    
    sub rsp, 020h       ; shadow space ; 阴影空间
    call HvReturnStackPointerForVmxoff
    add rsp, 020h       ; remove for shadow space ; 移除用于阴影空间的部分

    mov [rsp+088h], rax  ; now, rax contains rsp ; 现在，rax中包含了rsp的值

    sub rsp, 020h       ; shadow space ; 阴影空间
    call HvReturnInstructionPointerForVmxoff
    add rsp, 020h       ; remove for shadow space ; 移除用于阴影空间的部分

    mov rdx, rsp        ; save current rsp ; 保存当前的rsp值

    mov rbx, [rsp+088h] ; read rsp again ; 再次读取rsp值

    mov rsp, rbx

    push rax            ; push the return address as we changed the stack, we push
                        ; it to the new stack 
                        ; 由于我们更改了堆栈，将返回地址推送到新的堆栈中

    mov rsp, rdx        ; restore previous rsp ; 恢复之前的rsp值
                        
    sub rbx,08h         ; we push sth, so we have to add (sub) +8 from previous stack
                        ; also rbx already contains the rsp
                        ; 我们推送了一些内容，所以我们需要在之前的堆栈上添加（或减去）+8；此外，rbx已经包含了rsp的值
    mov [rsp+088h], rbx ; move the new pointer to the current stack  ; 将新指针移动到当前堆栈上

	RestoreState:

	pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp		         ; rsp
    pop rbp
    pop rsi
    pop rdi 
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    popfq

	pop		rsp     ; restore rsp ; 恢复rsp值
	ret             ; jump back to where we called Vmcall ; 跳回到调用Vmcall的位置

AsmVmxoffHandler ENDP

;------------------------------------------------------------------------

END
