
a.out:     file format mach-o-x86-64


Disassembly of section .text:

0000000100003f60 <_func>:
   100003f60:	55                   	push   rbp
   100003f61:	48 89 e5             	mov    rbp,rsp
   100003f64:	89 7d fc             	mov    DWORD PTR [rbp-0x4],edi
   100003f67:	89 75 f8             	mov    DWORD PTR [rbp-0x8],esi
   100003f6a:	8b 45 fc             	mov    eax,DWORD PTR [rbp-0x4]
   100003f6d:	03 45 f8             	add    eax,DWORD PTR [rbp-0x8]
   100003f70:	5d                   	pop    rbp
   100003f71:	c3                   	ret    
   100003f72:	66 2e 0f 1f 84 00 00 	nop    WORD PTR cs:[rax+rax*1+0x0]
   100003f79:	00 00 00 
   100003f7c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]

0000000100003f80 <_main>:
   100003f80:	55                   	push   rbp
   100003f81:	48 89 e5             	mov    rbp,rsp
   100003f84:	48 83 ec 10          	sub    rsp,0x10
   100003f88:	c7 45 fc 0a 00 00 00 	mov    DWORD PTR [rbp-0x4],0xa
   100003f8f:	c7 45 f8 14 00 00 00 	mov    DWORD PTR [rbp-0x8],0x14
   100003f96:	8b 7d fc             	mov    edi,DWORD PTR [rbp-0x4]
   100003f99:	8b 75 f8             	mov    esi,DWORD PTR [rbp-0x8]
   100003f9c:	e8 bf ff ff ff       	call   100003f60 <_func>
   100003fa1:	31 c9                	xor    ecx,ecx
   100003fa3:	89 45 f4             	mov    DWORD PTR [rbp-0xc],eax
   100003fa6:	89 c8                	mov    eax,ecx
   100003fa8:	48 83 c4 10          	add    rsp,0x10
   100003fac:	5d                   	pop    rbp
   100003fad:	c3                   	ret    

Disassembly of section __TEXT.__unwind_info:

0000000100003fb0 <__TEXT.__unwind_info>:
   100003fb0:	01 00                	add    DWORD PTR [rax],eax
   100003fb2:	00 00                	add    BYTE PTR [rax],al
   100003fb4:	1c 00                	sbb    al,0x0
   100003fb6:	00 00                	add    BYTE PTR [rax],al
   100003fb8:	00 00                	add    BYTE PTR [rax],al
   100003fba:	00 00                	add    BYTE PTR [rax],al
   100003fbc:	1c 00                	sbb    al,0x0
   100003fbe:	00 00                	add    BYTE PTR [rax],al
   100003fc0:	00 00                	add    BYTE PTR [rax],al
   100003fc2:	00 00                	add    BYTE PTR [rax],al
   100003fc4:	1c 00                	sbb    al,0x0
   100003fc6:	00 00                	add    BYTE PTR [rax],al
   100003fc8:	02 00                	add    al,BYTE PTR [rax]
   100003fca:	00 00                	add    BYTE PTR [rax],al
   100003fcc:	60                   	(bad)  
   100003fcd:	3f                   	(bad)  
   100003fce:	00 00                	add    BYTE PTR [rax],al
   100003fd0:	34 00                	xor    al,0x0
   100003fd2:	00 00                	add    BYTE PTR [rax],al
   100003fd4:	34 00                	xor    al,0x0
   100003fd6:	00 00                	add    BYTE PTR [rax],al
   100003fd8:	af                   	scas   eax,DWORD PTR es:[rdi]
   100003fd9:	3f                   	(bad)  
   100003fda:	00 00                	add    BYTE PTR [rax],al
   100003fdc:	00 00                	add    BYTE PTR [rax],al
   100003fde:	00 00                	add    BYTE PTR [rax],al
   100003fe0:	34 00                	xor    al,0x0
   100003fe2:	00 00                	add    BYTE PTR [rax],al
   100003fe4:	03 00                	add    eax,DWORD PTR [rax]
   100003fe6:	00 00                	add    BYTE PTR [rax],al
   100003fe8:	0c 00                	or     al,0x0
   100003fea:	01 00                	add    DWORD PTR [rax],eax
   100003fec:	10 00                	adc    BYTE PTR [rax],al
   100003fee:	01 00                	add    DWORD PTR [rax],eax
   100003ff0:	00 00                	add    BYTE PTR [rax],al
   100003ff2:	00 00                	add    BYTE PTR [rax],al
   100003ff4:	00 00                	add    BYTE PTR [rax],al
   100003ff6:	00 01                	add    BYTE PTR [rcx],al
