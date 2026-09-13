.data
    EXTERN NtUserSendInput_SSN : DWORD


.code

Hagi_ReadVirtualMemory PROC
	mov r10, rcx
	mov eax, 63
	syscall
	ret
Hagi_ReadVirtualMemory ENDP

Hagi_WriteVirtualMemory PROC
	mov r10, rcx
	mov eax, 58
	syscall
	ret
Hagi_WriteVirtualMemory ENDP

; wrapper for ntusersendinput syscall
Hagi_NtUserSendInput PROC
    mov r10, rcx
    mov eax, NtUserSendInput_SSN
    syscall
    ret
Hagi_NtUserSendInput ENDP

END