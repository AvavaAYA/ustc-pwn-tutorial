section	.text
	global _start       ;must be declared for using gcc
_start:                     ;tell linker entry point
    push    0x61616161
    mov     ebx,0x1 
    mov     ecx,esp
    mov     edx,4
    mov     eax,4
    int     0x80
    mov     eax,1
    int     0x80