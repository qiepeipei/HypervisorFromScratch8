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
            ; ���ǿ��ܴ��ڷǶ���Ķ�ջ״̬����˶�ջ֮ǰ���ڴ���ܻ�����IRQLС�ڻ���ڴ��������������һЩ����ռ�������������

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

	mov rcx, rsp		; Fast call argument to PGUEST_REGS ; PGUEST_REGS�Ŀ��ٵ��ò���
	sub	rsp, 28h		; Free some space for Shadow Section ; ΪShadow Section�ڳ�һЩ�ռ�
	call	VmxVmexitHandler
	add	rsp, 28h		; Restore the state ; �ָ�״̬

	cmp	al, 1	; Check whether we have to turn off VMX or Not (the result is in RAX) ; ����Ƿ���Ҫ�ر�VMX������洢��RAX�У�
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

	sub rsp, 0100h      ; to avoid error in future functions ; Ϊ�˱��⽫�������еĴ���
	jmp VmxVmresume

AsmVmexitHandler ENDP

;------------------------------------------------------------------------

AsmVmxoffHandler PROC
    
    sub rsp, 020h       ; shadow space ; ��Ӱ�ռ�
    call HvReturnStackPointerForVmxoff
    add rsp, 020h       ; remove for shadow space ; �Ƴ�������Ӱ�ռ�Ĳ���

    mov [rsp+088h], rax  ; now, rax contains rsp ; ���ڣ�rax�а�����rsp��ֵ

    sub rsp, 020h       ; shadow space ; ��Ӱ�ռ�
    call HvReturnInstructionPointerForVmxoff
    add rsp, 020h       ; remove for shadow space ; �Ƴ�������Ӱ�ռ�Ĳ���

    mov rdx, rsp        ; save current rsp ; ���浱ǰ��rspֵ

    mov rbx, [rsp+088h] ; read rsp again ; �ٴζ�ȡrspֵ

    mov rsp, rbx

    push rax            ; push the return address as we changed the stack, we push
                        ; it to the new stack 
                        ; �������Ǹ����˶�ջ�������ص�ַ���͵��µĶ�ջ��

    mov rsp, rdx        ; restore previous rsp ; �ָ�֮ǰ��rspֵ
                        
    sub rbx,08h         ; we push sth, so we have to add (sub) +8 from previous stack
                        ; also rbx already contains the rsp
                        ; ����������һЩ���ݣ�����������Ҫ��֮ǰ�Ķ�ջ����ӣ����ȥ��+8�����⣬rbx�Ѿ�������rsp��ֵ
    mov [rsp+088h], rbx ; move the new pointer to the current stack  ; ����ָ���ƶ�����ǰ��ջ��

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

	pop		rsp     ; restore rsp ; �ָ�rspֵ
	ret             ; jump back to where we called Vmcall ; ���ص�����Vmcall��λ��

AsmVmxoffHandler ENDP

;------------------------------------------------------------------------

END
