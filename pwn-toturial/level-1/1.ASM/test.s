push rbp
mov rbp,rsp
mov rax,1
push 0xa636261
mov rdi,1
mov rsi,rsp
mov rdx,4
syscall
leave
ret
