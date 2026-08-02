## Un análisis con un nuevo enfoque

Después de una semana de mucho trabajo, ayer decidí desconectar del todo. Al llegar la noche, me apetecía hacer algo, así que me puse a resolver crackmes para entrenar el reconocimiento de patrones (sin buscar un análisis exhaustivo, porque tampoco era la hora ni el estado mental para eso). Este writeup surge de justamente eso: al ser ya ciertas horas y ver que mi cerebro no estaba para análisis kilométricos ni conocimiento en profundidad, decidí enfocarlo en la optimización; simplemente resolver estos pequeños puzzles para saltar al siguiente.

Este writeup tiene un nuevo enfoque que también me parece valioso, y es el motivo por el que lo pongo aquí: identificar patrones rápidamente, entender qué hace el binario por detrás, identificar un objetivo de "ataque" y ejecutar dicho ataque. Hoy no será un análisis kilométrico como los otros de este directorio; será un writeup más bien pensado para que, si algún día algún lector que esté empezando lee esto, pueda pensar en cómo identificar lo que busca "bajo presión". 

Dicho esto (aunque me repita), hoy no veremos las mismas estructuras que vimos y que están mejor explicadas en otros writeups (si tuviera que recomendar alguno, sería el de "u-can't-pass" en crackmes o el del segundo análisis en binary-analysis). Si quisieras metodología, te recomiendo que vayas mejor a los otros. 

Este es el crackme a analizar, se llama "access me please" de chaltu: [Access me please](https://crackmes.one/crackme/6a3a7734a4b247348ae80666).


### Estructura

Después de identificar ciertas propiedades del binario con el comando `file`, este es el output que vemos con el comando strings:

```bash
/lib64/ld-linux-x86-64.so.2
puts
__stack_chk_fail
__isoc23_scanf
__libc_start_main
__cxa_finalize
printf
libc.so.6
GLIBC_2.38
GLIBC_2.4
GLIBC_2.2.5
GLIBC_2.34
_ITM_deregisterTMCloneTable
__gmon_start__
_ITM_registerTMCloneTable
PTE1
u3UH
Access Granted
Enter password: 
Access Denied!!!!
;*3$"
GCC: (GNU) 16.1.1 20260430
Scrt1.o
__abi_tag
crtbeginS.o
deregister_tm_clones
__do_global_dtors_aux
completed.0
__do_global_dtors_aux_fini_array_entry
frame_dummy
__frame_dummy_init_array_entry
main.c
crtendS.o
__FRAME_END__
_DYNAMIC
__GNU_EH_FRAME_HDR
_GLOBAL_OFFSET_TABLE_
__libc_start_main@GLIBC_2.34
_ITM_deregisterTMCloneTable
puts@GLIBC_2.2.5
_edata
_fini
__stack_chk_fail@GLIBC_2.4
printf@GLIBC_2.2.5
__isoc23_scanf@GLIBC_2.38
__data_start
__gmon_start__
__dso_handle
_IO_stdin_used
_end
__bss_start
main
__TMC_END__
_ITM_registerTMCloneTable
__cxa_finalize@GLIBC_2.2.5
_init
canaccess
printaccess
.symtab
.strtab
.shstrtab
.note.gnu.build-id
.interp
.gnu.hash
.dynsym
.dynstr
.gnu.version
.gnu.version_r
.rela.dyn
.rela.plt
.init
.text
.fini
.rodata
.eh_frame_hdr
.eh_frame
.sframe
.note.gnu.property
.note.ABI-tag
.init_array
.fini_array
.dynamic
.got
.got.plt
.data
.bss
.comment
```

Después de un repaso rápido y dejando de lado las secciones o las funciones de librerías y métodos de compilación, podemos identificar los strings hardcoded que ya nos dan información valiosa sobre el contenido del mismo:

```bash
Access Granted
Enter password: 
Access Denied!!!!
canaccess
printaccess
```

Las dos que podemos ver al final nos hacen pensar en un par de funciones que va a contener el código fuente más que en la contraseña hardcoded. Además, estando la función scanf (lo cual nos indica que el binario va a recibir input del usuario) refuerzan más esto. Aun así, lo confirmaremos con el análisis estático.

Dicho esto, si quisiéramos, podríamos usar el comando `readelf` con las flags que normalmente comento, de las cuales hoy no voy a poner el output. 

Pasemos al análisis estático, aunque sea por encima.

### Análisis estático

Para este tipo de binarios no usaremos Ghidra todavía. No es por pereza ni vagancia, el simple hecho de usar Ghidra aunque sea para un binario tan pequeño nos permitiría familiarizarnos más con la interfaz. Pero no la uso en los writeups que hay hasta el momento por el simple hecho de que Ghidra se puede usar para que te lo dé todo mascado, no queremos eso. El uso que le daré será para binarios grandes donde su ayuda nos vendrá genial y permitirá optimizar muchísima faena y abstracción.

```bash

main:     formato del fichero elf64-x86-64


Desensamblado de la sección .init:

0000000000001000 <_init>:
    1000:	f3 0f 1e fa          	endbr64
    1004:	48 83 ec 08          	sub    rsp,0x8
    1008:	48 8b 05 c1 2f 00 00 	mov    rax,QWORD PTR [rip+0x2fc1]        # 3fd0 <__gmon_start__>
    100f:	48 85 c0             	test   rax,rax
    1012:	74 02                	je     1016 <_init+0x16>
    1014:	ff d0                	call   rax
    1016:	48 83 c4 08          	add    rsp,0x8
    101a:	c3                   	ret

Desensamblado de la sección .plt:

0000000000001020 <puts@plt-0x10>:
    1020:	ff 35 ca 2f 00 00    	push   QWORD PTR [rip+0x2fca]        # 3ff0 <_GLOBAL_OFFSET_TABLE_+0x8>
    1026:	ff 25 cc 2f 00 00    	jmp    QWORD PTR [rip+0x2fcc]        # 3ff8 <_GLOBAL_OFFSET_TABLE_+0x10>
    102c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]

0000000000001030 <puts@plt>:
    1030:	ff 25 ca 2f 00 00    	jmp    QWORD PTR [rip+0x2fca]        # 4000 <puts@GLIBC_2.2.5>
    1036:	68 00 00 00 00       	push   0x0
    103b:	e9 e0 ff ff ff       	jmp    1020 <_init+0x20>

0000000000001040 <__stack_chk_fail@plt>:
    1040:	ff 25 c2 2f 00 00    	jmp    QWORD PTR [rip+0x2fc2]        # 4008 <__stack_chk_fail@GLIBC_2.4>
    1046:	68 01 00 00 00       	push   0x1
    104b:	e9 d0 ff ff ff       	jmp    1020 <_init+0x20>

0000000000001050 <printf@plt>:
    1050:	ff 25 ba 2f 00 00    	jmp    QWORD PTR [rip+0x2fba]        # 4010 <printf@GLIBC_2.2.5>
    1056:	68 02 00 00 00       	push   0x2
    105b:	e9 c0 ff ff ff       	jmp    1020 <_init+0x20>

0000000000001060 <__isoc23_scanf@plt>:
    1060:	ff 25 b2 2f 00 00    	jmp    QWORD PTR [rip+0x2fb2]        # 4018 <__isoc23_scanf@GLIBC_2.38>
    1066:	68 03 00 00 00       	push   0x3
    106b:	e9 b0 ff ff ff       	jmp    1020 <_init+0x20>

Desensamblado de la sección .text:

0000000000001070 <_start>:
    1070:	f3 0f 1e fa          	endbr64
    1074:	31 ed                	xor    ebp,ebp
    1076:	49 89 d1             	mov    r9,rdx
    1079:	5e                   	pop    rsi
    107a:	48 89 e2             	mov    rdx,rsp
    107d:	48 83 e4 f0          	and    rsp,0xfffffffffffffff0
    1081:	50                   	push   rax
    1082:	54                   	push   rsp
    1083:	45 31 c0             	xor    r8d,r8d
    1086:	31 c9                	xor    ecx,ecx
    1088:	48 8d 3d fb 00 00 00 	lea    rdi,[rip+0xfb]        # 118a <main>
    108f:	ff 15 2b 2f 00 00    	call   QWORD PTR [rip+0x2f2b]        # 3fc0 <__libc_start_main@GLIBC_2.34>
    1095:	f4                   	hlt
    1096:	66 2e 0f 1f 84 00 00 	cs nop WORD PTR [rax+rax*1+0x0]
    109d:	00 00 00 

00000000000010a0 <deregister_tm_clones>:
    10a0:	48 8d 3d 89 2f 00 00 	lea    rdi,[rip+0x2f89]        # 4030 <__TMC_END__>
    10a7:	48 8d 05 82 2f 00 00 	lea    rax,[rip+0x2f82]        # 4030 <__TMC_END__>
    10ae:	48 39 f8             	cmp    rax,rdi
    10b1:	74 15                	je     10c8 <deregister_tm_clones+0x28>
    10b3:	48 8b 05 0e 2f 00 00 	mov    rax,QWORD PTR [rip+0x2f0e]        # 3fc8 <_ITM_deregisterTMCloneTable>
    10ba:	48 85 c0             	test   rax,rax
    10bd:	74 09                	je     10c8 <deregister_tm_clones+0x28>
    10bf:	ff e0                	jmp    rax
    10c1:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
    10c8:	c3                   	ret
    10c9:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]

00000000000010d0 <register_tm_clones>:
    10d0:	48 8d 3d 59 2f 00 00 	lea    rdi,[rip+0x2f59]        # 4030 <__TMC_END__>
    10d7:	48 8d 35 52 2f 00 00 	lea    rsi,[rip+0x2f52]        # 4030 <__TMC_END__>
    10de:	48 29 fe             	sub    rsi,rdi
    10e1:	48 89 f0             	mov    rax,rsi
    10e4:	48 c1 ee 3f          	shr    rsi,0x3f
    10e8:	48 c1 f8 03          	sar    rax,0x3
    10ec:	48 01 c6             	add    rsi,rax
    10ef:	48 d1 fe             	sar    rsi,1
    10f2:	74 14                	je     1108 <register_tm_clones+0x38>
    10f4:	48 8b 05 dd 2e 00 00 	mov    rax,QWORD PTR [rip+0x2edd]        # 3fd8 <_ITM_registerTMCloneTable>
    10fb:	48 85 c0             	test   rax,rax
    10fe:	74 08                	je     1108 <register_tm_clones+0x38>
    1100:	ff e0                	jmp    rax
    1102:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    1108:	c3                   	ret
    1109:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]

0000000000001110 <__do_global_dtors_aux>:
    1110:	f3 0f 1e fa          	endbr64
    1114:	80 3d 15 2f 00 00 00 	cmp    BYTE PTR [rip+0x2f15],0x0        # 4030 <__TMC_END__>
    111b:	75 33                	jne    1150 <__do_global_dtors_aux+0x40>
    111d:	55                   	push   rbp
    111e:	48 83 3d ba 2e 00 00 	cmp    QWORD PTR [rip+0x2eba],0x0        # 3fe0 <__cxa_finalize@GLIBC_2.2.5>
    1125:	00 
    1126:	48 89 e5             	mov    rbp,rsp
    1129:	74 0d                	je     1138 <__do_global_dtors_aux+0x28>
    112b:	48 8b 3d f6 2e 00 00 	mov    rdi,QWORD PTR [rip+0x2ef6]        # 4028 <__dso_handle>
    1132:	ff 15 a8 2e 00 00    	call   QWORD PTR [rip+0x2ea8]        # 3fe0 <__cxa_finalize@GLIBC_2.2.5>
    1138:	e8 63 ff ff ff       	call   10a0 <deregister_tm_clones>
    113d:	c6 05 ec 2e 00 00 01 	mov    BYTE PTR [rip+0x2eec],0x1        # 4030 <__TMC_END__>
    1144:	5d                   	pop    rbp
    1145:	c3                   	ret
    1146:	66 2e 0f 1f 84 00 00 	cs nop WORD PTR [rax+rax*1+0x0]
    114d:	00 00 00 
    1150:	c3                   	ret
    1151:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
    1155:	66 66 2e 0f 1f 84 00 	data16 cs nop WORD PTR [rax+rax*1+0x0]
    115c:	00 00 00 00 

0000000000001160 <frame_dummy>:
    1160:	f3 0f 1e fa          	endbr64
    1164:	e9 67 ff ff ff       	jmp    10d0 <register_tm_clones>

0000000000001169 <canaccess>:
    1169:	55                   	push   rbp
    116a:	48 89 e5             	mov    rbp,rsp
    116d:	b8 01 00 00 00       	mov    eax,0x1
    1172:	5d                   	pop    rbp
    1173:	c3                   	ret

0000000000001174 <printaccess>:
    1174:	55                   	push   rbp
    1175:	48 89 e5             	mov    rbp,rsp
    1178:	48 8d 05 85 0e 00 00 	lea    rax,[rip+0xe85]        # 2004 <_IO_stdin_used+0x4>
    117f:	48 89 c7             	mov    rdi,rax
    1182:	e8 a9 fe ff ff       	call   1030 <puts@plt>
    1187:	90                   	nop
    1188:	5d                   	pop    rbp
    1189:	c3                   	ret

000000000000118a <main>:
    118a:	55                   	push   rbp
    118b:	48 89 e5             	mov    rbp,rsp
    118e:	48 83 ec 10          	sub    rsp,0x10
    1192:	64 48 8b 04 25 28 00 	mov    rax,QWORD PTR fs:0x28
    1199:	00 00 
    119b:	48 89 45 f8          	mov    QWORD PTR [rbp-0x8],rax
    119f:	31 c0                	xor    eax,eax
    11a1:	48 8d 05 6b 0e 00 00 	lea    rax,[rip+0xe6b]        # 2013 <_IO_stdin_used+0x13>
    11a8:	48 89 c7             	mov    rdi,rax
    11ab:	b8 00 00 00 00       	mov    eax,0x0
    11b0:	e8 9b fe ff ff       	call   1050 <printf@plt>
    11b5:	48 8d 45 f4          	lea    rax,[rbp-0xc]
    11b9:	48 8d 15 64 0e 00 00 	lea    rdx,[rip+0xe64]        # 2024 <_IO_stdin_used+0x24>
    11c0:	48 89 c6             	mov    rsi,rax
    11c3:	48 89 d7             	mov    rdi,rdx
    11c6:	b8 00 00 00 00       	mov    eax,0x0
    11cb:	e8 90 fe ff ff       	call   1060 <__isoc23_scanf@plt>
    11d0:	8b 45 f4             	mov    eax,DWORD PTR [rbp-0xc]
    11d3:	3d b5 b7 00 00       	cmp    eax,0xb7b5
    11d8:	75 10                	jne    11ea <main+0x60>
    11da:	e8 8a ff ff ff       	call   1169 <canaccess>
    11df:	84 c0                	test   al,al
    11e1:	74 16                	je     11f9 <main+0x6f>
    11e3:	e8 8c ff ff ff       	call   1174 <printaccess>
    11e8:	eb 0f                	jmp    11f9 <main+0x6f>
    11ea:	48 8d 05 36 0e 00 00 	lea    rax,[rip+0xe36]        # 2027 <_IO_stdin_used+0x27>
    11f1:	48 89 c7             	mov    rdi,rax
    11f4:	e8 37 fe ff ff       	call   1030 <puts@plt>
    11f9:	b8 00 00 00 00       	mov    eax,0x0
    11fe:	48 8b 55 f8          	mov    rdx,QWORD PTR [rbp-0x8]
    1202:	64 48 2b 14 25 28 00 	sub    rdx,QWORD PTR fs:0x28
    1209:	00 00 
    120b:	74 05                	je     1212 <main+0x88>
    120d:	e8 2e fe ff ff       	call   1040 <__stack_chk_fail@plt>
    1212:	c9                   	leave
    1213:	c3                   	ret

Desensamblado de la sección .fini:

0000000000001214 <_fini>:
    1214:	f3 0f 1e fa          	endbr64
    1218:	48 83 ec 08          	sub    rsp,0x8
    121c:	48 83 c4 08          	add    rsp,0x8
    1220:	c3                   	ret
```

Ya podemos confirmar con certeza que eran dos funciones (canaccess y printaccess) vistas anteriormente con el comando strings. Recordemos que es little-endian (no lo he mostrado pero se puede confirmar con el comando file y con el comando readelf -h), para que no nos despiste lo que haremos posteriormente.

### Análisis dinámico

En este caso, aunque ya vemos toda la información relevante con el análisis estático, vamos a abrir GDB como hacemos normalmente:

```text
(gdb) disassemble main 
Dump of assembler code for function main:
   0x000000000000118a <+0>:	push   rbp
   0x000000000000118b <+1>:	mov    rbp,rsp
   0x000000000000118e <+4>:	sub    rsp,0x10
   0x0000000000001192 <+8>:	mov    rax,QWORD PTR fs:0x28
   0x000000000000119b <+17>:	mov    QWORD PTR [rbp-0x8],rax
   0x000000000000119f <+21>:	xor    eax,eax
   0x00000000000011a1 <+23>:	lea    rax,[rip+0xe6b]        # 0x2013
   0x00000000000011a8 <+30>:	mov    rdi,rax
   0x00000000000011ab <+33>:	mov    eax,0x0
   0x00000000000011b0 <+38>:	call   0x1050 <printf@plt>
   0x00000000000011b5 <+43>:	lea    rax,[rbp-0xc]
   0x00000000000011b9 <+47>:	lea    rdx,[rip+0xe64]        # 0x2024
   0x00000000000011c0 <+54>:	mov    rsi,rax
   0x00000000000011c3 <+57>:	mov    rdi,rdx
   0x00000000000011c6 <+60>:	mov    eax,0x0
   0x00000000000011cb <+65>:	call   0x1060 <__isoc23_scanf@plt>
   0x00000000000011d0 <+70>:	mov    eax,DWORD PTR [rbp-0xc]
   0x00000000000011d3 <+73>:	cmp    eax,0xb7b5
   0x00000000000011d8 <+78>:	jne    0x11ea <main+96>
   0x00000000000011da <+80>:	call   0x1169 <canaccess>
   0x00000000000011df <+85>:	test   al,al
   0x00000000000011e1 <+87>:	je     0x11f9 <main+111>
   0x00000000000011e3 <+89>:	call   0x1174 <printaccess>
   0x00000000000011e8 <+94>:	jmp    0x11f9 <main+111>
   0x00000000000011ea <+96>:	lea    rax,[rip+0xe36]        # 0x2027
   0x00000000000011f1 <+103>:	mov    rdi,rax
   0x00000000000011f4 <+106>:	call   0x1030 <puts@plt>
   0x00000000000011f9 <+111>:	mov    eax,0x0
   0x00000000000011fe <+116>:	mov    rdx,QWORD PTR [rbp-0x8]
   0x0000000000001202 <+120>:	sub    rdx,QWORD PTR fs:0x28
   0x000000000000120b <+129>:	je     0x1212 <main+136>
   0x000000000000120d <+131>:	call   0x1040 <__stack_chk_fail@plt>
   0x0000000000001212 <+136>:	leave
   0x0000000000001213 <+137>:	ret
End of assembler dump.
```

Lo que hemos hecho en primera instancia es comprobar que contiene cada parte que pertenece a la sección .rodata, para la cual voy a poner el output del comando `info file` usado en gdb que nos aporta mucha información útil sobre las secciones:

```text
(gdb) info file
Symbols from "/home/ygm/crackmes/access-me-please/main".
Local exec file:
	`/home/ygm/crackmes/access-me-please/main', file type elf64-x86-64.
	Entry point: 0x1070
	0x0000000000000388 - 0x00000000000003ac is .note.gnu.build-id
	0x00000000000003ac - 0x00000000000003c8 is .interp
	0x00000000000003c8 - 0x00000000000003e4 is .gnu.hash
	0x00000000000003e8 - 0x00000000000004d8 is .dynsym
	0x00000000000004d8 - 0x00000000000005a1 is .dynstr
	0x00000000000005a2 - 0x00000000000005b6 is .gnu.version
	0x00000000000005b8 - 0x0000000000000608 is .gnu.version_r
	0x0000000000000608 - 0x00000000000006c8 is .rela.dyn
	0x00000000000006c8 - 0x0000000000000728 is .rela.plt
	0x0000000000001000 - 0x000000000000101b is .init
	0x0000000000001020 - 0x0000000000001070 is .plt
	0x0000000000001070 - 0x0000000000001214 is .text
	0x0000000000001214 - 0x0000000000001221 is .fini
	0x0000000000002000 - 0x0000000000002039 is .rodata
	0x000000000000203c - 0x0000000000002070 is .eh_frame_hdr
	0x0000000000002070 - 0x000000000000212c is .eh_frame
	0x0000000000002130 - 0x000000000000219c is .sframe
	0x00000000000021b8 - 0x00000000000021f8 is .note.gnu.property
	0x00000000000021f8 - 0x0000000000002218 is .note.ABI-tag
	0x0000000000003dd0 - 0x0000000000003dd8 is .init_array
	0x0000000000003dd8 - 0x0000000000003de0 is .fini_array
	0x0000000000003de0 - 0x0000000000003fc0 is .dynamic
	0x0000000000003fc0 - 0x0000000000003fe8 is .got
	0x0000000000003fe8 - 0x0000000000004020 is .got.plt
	0x0000000000004020 - 0x0000000000004030 is .data
	0x0000000000004030 - 0x0000000000004038 is .bss
```

Con esto ya sabemos que todas las direcciones que ya ha calculado GDB (detrás del hashtag) pertenecen a .rodata, así que vamos a analizar cada una a ver que contiene para mapearlo directamente:

```text
(gdb) x/s 0x2013
0x2013:	"Enter password: "
(gdb) x/s 0x2024
0x2024:	"%d"
(gdb) x/s 0x2027
0x2027:	"Access Denied!!!!"
```

Esta es la parte más reveladora de todas. Sabemos que está la función scanf en el código fuente y, además, fijémonos en la segunda instrucción: `%d`. Nos está diciendo que lo que espera recibir es un entero.

Sabiendo esto, mirando rápidamente el desensamblado muy por encima, podemos ver casi al instante (casi incluso sin habernos fijado en la instrucción como tal) lo que destaca:

```text
 0x00000000000011d3 <+73>:	cmp    eax, 0xb7b5
                                        |_____|
                                            
                                        Esta parte

```

Si hacemos la conversión rápida, veremos que nos da el número 47029. Al ejecutar el binario, veamos lo que obtenemos:

```bash
./main 
Enter password: 47029
Access Granted
```

Quiero añadir una pequeña comprobación con un número aleatorio:

```bash
./main 
Enter password: 4656
Access Denied!!!!
```

Como vemos, hemos podido identificar la finalidad del binario sin haber leído casi Assembly. Pese a que (obviamente) es un binario muy sencillo y beginner friendly, lo que quería mostrar con esto son un par de cosas: la manera de identificar el objetivo, el cómo pensar y cómo proceder. Pero no nos vamos a quedar aquí, vamos a intentar evadir ciertas comprobaciones del binario para luego parchearlo a nivel de bytes con un editor hexadecimal.

A esta parte no la llamaría evasión en el sentido estricto, simplemente le suministramos al programa el dato correcto y dejamos que se ejecute con normalidad. Pero nos sirve para practicar con GDB, así que la pongo:

```text
(gdb) break *main+60
Breakpoint 1 at 0x11c6
(gdb) run
Starting program: /home/ygm/crackmes/access-me-please/main 
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".

Breakpoint 1, 0x00005555555551c6 in main ()
(gdb) set variable *(int *)($rbp-0xc) = 47029
(gdb) jump *main+70
Continuing at 0x5555555551d0.
Enter password: Access Granted
[Inferior 1 (process 8279) exited normally]
```

Y ahora, vamos a la parte que, personalmente, me causa más diversión. Vamos a parchear el binario para no tener siquiera que recordar el número que nos pide. Primero de todo, vamos a abrir el binario con el comando `nvim -b`, después de haberlo copiado en el mismo directorio (más que nada por si algún fallo surgiera, tuviéramos el original sin necesidad de volver a descargarlo).

Al estar dentro (por ejemplo, nvim -b main_patched), veremos mucha data ilegible como ya sabemos. El comando a usar ahora es este para que nos muestre la traducción del binario en hexadecimal: `:%!xxd`.

Antes de seguir, vayamos a identificar los bytes de la instrucción y qué deberíamos ver (ya que sabemos que xxd junta en bloques de 16 bytes, por lo que buscaremos mediante una aproximación y los bytes al revés al ser little-endian):

```bash
objdump -d -M intel main_patched | grep -C1 "11d3"
    11d0:	8b 45 f4             	mov    eax,DWORD PTR [rbp-0xc]
    11d3:	3d b5 b7 00 00       	cmp    eax,0xb7b5
    |_____________________|
    
    Esta parte es la que nos interesa
        

    11d8:	75 10                	jne    11ea <main+0x60>
```

Teniendo esto, simplemente filtramos con el binario abierto en el editor de texto pero convertido a hexadecimal. Después, cambiamos la secuencia de bytes a 0000. Esta es la secuencia encontrada:

```text
000011d0: 8b45 f43d b5b7 0000 7510 e88a ffff ff84  .E.=....u.......
                    |__|

                    Cambiamos la secuencia entera por ceros, quedando ese par de bytes como 0000

```

Posteriormente aplicamos los cambios al binario con el comando `:%!xxd -r` y guardamos con `:wq`.

Ahora, al ejecutar el binario veamos qué ocurre:

```text
./main_patched 
Enter password: 0
Access Granted
```

¡Lo hemos logrado! La verdad es que esta parte es brutal, pero no te vayas todavía, que en el siguiente vamos a cambiar un opcode (lo siento por el spoiler). 

Aun así, como me gusta ser riguroso, vamos a hacer las comprobaciones para que veamos si todo ha ido correctamente sobre ambos binarios con el par de comprobaciones:

```bash
./main_patched 
Enter password: 0
Access Granted

./main_patched
Enter password: 47029
Access Denied!!!!

./main
Enter password: 47029
Access Granted

./main
Enter password: 0
Access Denied!!!!
```


## Segundo binario: invirtiendo un salto en vez de un valor

Este es el binario "level2" de nimacpp, aquí te lo dejo: [level2](https://crackmes.one/crackme/65da0fce6d3d2b1fef4be4df). Vamos a trabajar sobre este ahora. 

### Estructura

Otra vez, y creo que es importante remarcarlo aunque no vaya a poner ciertos outputs por optimización y velocidad, hay que usar los comandos y metodología pertinente, para interiorizarla y aprender. 

Así que, después de haberle pasado el comando file a este binario, vamos a mostrar el output de strings:

```bash
/lib64/ld-linux-x86-64.so.2
__cxa_finalize
__libc_start_main
strcmp
printf
libc.so.6
GLIBC_2.2.5
GLIBC_2.34
_ITM_deregisterTMCloneTable
__gmon_start__
_ITM_registerTMCloneTable
PTE1
u+UH
01234567891
Jsfdb56d65a
%s is password
Look at it as cracker 
:*3$"
GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
Scrt1.o
__abi_tag
crtstuff.c
deregister_tm_clones
__do_global_dtors_aux
completed.0
__do_global_dtors_aux_fini_array_entry
frame_dummy
__frame_dummy_init_array_entry
sym.c
__FRAME_END__
_DYNAMIC
__GNU_EH_FRAME_HDR
_GLOBAL_OFFSET_TABLE_
__libc_start_main@GLIBC_2.34
_ITM_deregisterTMCloneTable
_edata
_fini
printf@GLIBC_2.2.5
__data_start
strcmp@GLIBC_2.2.5
__gmon_start__
__dso_handle
_IO_stdin_used
_end
__bss_start
main
__TMC_END__
_ITM_registerTMCloneTable
__cxa_finalize@GLIBC_2.2.5
_init
.symtab
.strtab
.shstrtab
.interp
.note.gnu.property
.note.gnu.build-id
.note.ABI-tag
.gnu.hash
.dynsym
.dynstr
.gnu.version
.gnu.version_r
.rela.dyn
.rela.plt
.init
.plt.got
.plt.sec
.text
.fini
.rodata
.eh_frame_hdr
.eh_frame
.init_array
.fini_array
.dynamic
.data
.bss
.comment
```

En este output también vemos demasiada información que nos hace pensar por dónde irán los tiros. Aparece la función printf, strcmp, no parece que haya una función aparte de la del main y, además, vemos esto:

```text
01234567891
Jsfdb56d65a
%s is password
Look at it as cracker 
```

Lo que había comentado en otros writeups, aunque pueda depender del compilador, cuando veamos la función de printf podemos esperar un especificador de formato, que en este caso aparece aquí. Si viéramos la función puts, sabremos que es una optimización del compilador donde el string a imprimir con printf no espera recibir ningún argumento, por lo que no necesita parsearlo. 

Dicho esto, pasemos ahora al análisis estático (ni falta que hace decir que primero comprobemos todo con el comando readelf antes de nada).

### Análisis estático

Me gustaría decir que he analizado previamente el volcado con objdump para poder verificaros que solo existe el main en el código fuente, pero para que sea riguroso, voy a ponerlo todo para que lo podáis verificar también. Disculpad el exceso:

```bash
objdump -d -M intel level2 

level2:     formato del fichero elf64-x86-64


Desensamblado de la sección .init:

0000000000001000 <_init>:
    1000:	f3 0f 1e fa          	endbr64
    1004:	48 83 ec 08          	sub    rsp,0x8
    1008:	48 8b 05 d9 2f 00 00 	mov    rax,QWORD PTR [rip+0x2fd9]        # 3fe8 <__gmon_start__@Base>
    100f:	48 85 c0             	test   rax,rax
    1012:	74 02                	je     1016 <_init+0x16>
    1014:	ff d0                	call   rax
    1016:	48 83 c4 08          	add    rsp,0x8
    101a:	c3                   	ret

Desensamblado de la sección .plt:

0000000000001020 <.plt>:
    1020:	ff 35 92 2f 00 00    	push   QWORD PTR [rip+0x2f92]        # 3fb8 <_GLOBAL_OFFSET_TABLE_+0x8>
    1026:	f2 ff 25 93 2f 00 00 	bnd jmp QWORD PTR [rip+0x2f93]        # 3fc0 <_GLOBAL_OFFSET_TABLE_+0x10>
    102d:	0f 1f 00             	nop    DWORD PTR [rax]
    1030:	f3 0f 1e fa          	endbr64
    1034:	68 00 00 00 00       	push   0x0
    1039:	f2 e9 e1 ff ff ff    	bnd jmp 1020 <_init+0x20>
    103f:	90                   	nop
    1040:	f3 0f 1e fa          	endbr64
    1044:	68 01 00 00 00       	push   0x1
    1049:	f2 e9 d1 ff ff ff    	bnd jmp 1020 <_init+0x20>
    104f:	90                   	nop

Desensamblado de la sección .plt.got:

0000000000001050 <__cxa_finalize@plt>:
    1050:	f3 0f 1e fa          	endbr64
    1054:	f2 ff 25 9d 2f 00 00 	bnd jmp QWORD PTR [rip+0x2f9d]        # 3ff8 <__cxa_finalize@GLIBC_2.2.5>
    105b:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]

Desensamblado de la sección .plt.sec:

0000000000001060 <printf@plt>:
    1060:	f3 0f 1e fa          	endbr64
    1064:	f2 ff 25 5d 2f 00 00 	bnd jmp QWORD PTR [rip+0x2f5d]        # 3fc8 <printf@GLIBC_2.2.5>
    106b:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]

0000000000001070 <strcmp@plt>:
    1070:	f3 0f 1e fa          	endbr64
    1074:	f2 ff 25 55 2f 00 00 	bnd jmp QWORD PTR [rip+0x2f55]        # 3fd0 <strcmp@GLIBC_2.2.5>
    107b:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]

Desensamblado de la sección .text:

0000000000001080 <_start>:
    1080:	f3 0f 1e fa          	endbr64
    1084:	31 ed                	xor    ebp,ebp
    1086:	49 89 d1             	mov    r9,rdx
    1089:	5e                   	pop    rsi
    108a:	48 89 e2             	mov    rdx,rsp
    108d:	48 83 e4 f0          	and    rsp,0xfffffffffffffff0
    1091:	50                   	push   rax
    1092:	54                   	push   rsp
    1093:	45 31 c0             	xor    r8d,r8d
    1096:	31 c9                	xor    ecx,ecx
    1098:	48 8d 3d ca 00 00 00 	lea    rdi,[rip+0xca]        # 1169 <main>
    109f:	ff 15 33 2f 00 00    	call   QWORD PTR [rip+0x2f33]        # 3fd8 <__libc_start_main@GLIBC_2.34>
    10a5:	f4                   	hlt
    10a6:	66 2e 0f 1f 84 00 00 	cs nop WORD PTR [rax+rax*1+0x0]
    10ad:	00 00 00 

00000000000010b0 <deregister_tm_clones>:
    10b0:	48 8d 3d 59 2f 00 00 	lea    rdi,[rip+0x2f59]        # 4010 <__TMC_END__>
    10b7:	48 8d 05 52 2f 00 00 	lea    rax,[rip+0x2f52]        # 4010 <__TMC_END__>
    10be:	48 39 f8             	cmp    rax,rdi
    10c1:	74 15                	je     10d8 <deregister_tm_clones+0x28>
    10c3:	48 8b 05 16 2f 00 00 	mov    rax,QWORD PTR [rip+0x2f16]        # 3fe0 <_ITM_deregisterTMCloneTable@Base>
    10ca:	48 85 c0             	test   rax,rax
    10cd:	74 09                	je     10d8 <deregister_tm_clones+0x28>
    10cf:	ff e0                	jmp    rax
    10d1:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
    10d8:	c3                   	ret
    10d9:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]

00000000000010e0 <register_tm_clones>:
    10e0:	48 8d 3d 29 2f 00 00 	lea    rdi,[rip+0x2f29]        # 4010 <__TMC_END__>
    10e7:	48 8d 35 22 2f 00 00 	lea    rsi,[rip+0x2f22]        # 4010 <__TMC_END__>
    10ee:	48 29 fe             	sub    rsi,rdi
    10f1:	48 89 f0             	mov    rax,rsi
    10f4:	48 c1 ee 3f          	shr    rsi,0x3f
    10f8:	48 c1 f8 03          	sar    rax,0x3
    10fc:	48 01 c6             	add    rsi,rax
    10ff:	48 d1 fe             	sar    rsi,1
    1102:	74 14                	je     1118 <register_tm_clones+0x38>
    1104:	48 8b 05 e5 2e 00 00 	mov    rax,QWORD PTR [rip+0x2ee5]        # 3ff0 <_ITM_registerTMCloneTable@Base>
    110b:	48 85 c0             	test   rax,rax
    110e:	74 08                	je     1118 <register_tm_clones+0x38>
    1110:	ff e0                	jmp    rax
    1112:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    1118:	c3                   	ret
    1119:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]

0000000000001120 <__do_global_dtors_aux>:
    1120:	f3 0f 1e fa          	endbr64
    1124:	80 3d e5 2e 00 00 00 	cmp    BYTE PTR [rip+0x2ee5],0x0        # 4010 <__TMC_END__>
    112b:	75 2b                	jne    1158 <__do_global_dtors_aux+0x38>
    112d:	55                   	push   rbp
    112e:	48 83 3d c2 2e 00 00 	cmp    QWORD PTR [rip+0x2ec2],0x0        # 3ff8 <__cxa_finalize@GLIBC_2.2.5>
    1135:	00 
    1136:	48 89 e5             	mov    rbp,rsp
    1139:	74 0c                	je     1147 <__do_global_dtors_aux+0x27>
    113b:	48 8b 3d c6 2e 00 00 	mov    rdi,QWORD PTR [rip+0x2ec6]        # 4008 <__dso_handle>
    1142:	e8 09 ff ff ff       	call   1050 <__cxa_finalize@plt>
    1147:	e8 64 ff ff ff       	call   10b0 <deregister_tm_clones>
    114c:	c6 05 bd 2e 00 00 01 	mov    BYTE PTR [rip+0x2ebd],0x1        # 4010 <__TMC_END__>
    1153:	5d                   	pop    rbp
    1154:	c3                   	ret
    1155:	0f 1f 00             	nop    DWORD PTR [rax]
    1158:	c3                   	ret
    1159:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]

0000000000001160 <frame_dummy>:
    1160:	f3 0f 1e fa          	endbr64
    1164:	e9 77 ff ff ff       	jmp    10e0 <register_tm_clones>

0000000000001169 <main>:
    1169:	f3 0f 1e fa          	endbr64
    116d:	55                   	push   rbp
    116e:	48 89 e5             	mov    rbp,rsp
    1171:	48 83 ec 10          	sub    rsp,0x10
    1175:	48 8d 05 88 0e 00 00 	lea    rax,[rip+0xe88]        # 2004 <_IO_stdin_used+0x4>
    117c:	48 89 45 f8          	mov    QWORD PTR [rbp-0x8],rax
    1180:	48 8b 45 f8          	mov    rax,QWORD PTR [rbp-0x8]
    1184:	48 8d 15 85 0e 00 00 	lea    rdx,[rip+0xe85]        # 2010 <_IO_stdin_used+0x10>
    118b:	48 89 d6             	mov    rsi,rdx
    118e:	48 89 c7             	mov    rdi,rax
    1191:	e8 da fe ff ff       	call   1070 <strcmp@plt>
    1196:	85 c0                	test   eax,eax
    1198:	75 1d                	jne    11b7 <main+0x4e>
    119a:	48 8b 45 f8          	mov    rax,QWORD PTR [rbp-0x8]
    119e:	48 89 c6             	mov    rsi,rax
    11a1:	48 8d 05 74 0e 00 00 	lea    rax,[rip+0xe74]        # 201c <_IO_stdin_used+0x1c>
    11a8:	48 89 c7             	mov    rdi,rax
    11ab:	b8 00 00 00 00       	mov    eax,0x0
    11b0:	e8 ab fe ff ff       	call   1060 <printf@plt>
    11b5:	eb 14                	jmp    11cb <main+0x62>
    11b7:	48 8d 05 6d 0e 00 00 	lea    rax,[rip+0xe6d]        # 202b <_IO_stdin_used+0x2b>
    11be:	48 89 c7             	mov    rdi,rax
    11c1:	b8 00 00 00 00       	mov    eax,0x0
    11c6:	e8 95 fe ff ff       	call   1060 <printf@plt>
    11cb:	b8 00 00 00 00       	mov    eax,0x0
    11d0:	c9                   	leave
    11d1:	c3                   	ret

Desensamblado de la sección .fini:

00000000000011d4 <_fini>:
    11d4:	f3 0f 1e fa          	endbr64
    11d8:	48 83 ec 08          	sub    rsp,0x8
    11dc:	48 83 c4 08          	add    rsp,0x8
    11e0:	c3                   	ret
```

Dicho esto, pasemos al análisis dinámico.

### Análisis dinámico

Aquí tenemos la información sobre qué parte del binario pertenece a qué sección:

```text 
(gdb) info file
Symbols from "/home/ygm/crackmes/nimacpp-level2/level2".
Local exec file:
	`/home/ygm/crackmes/nimacpp-level2/level2', file type elf64-x86-64.
	Entry point: 0x1080
	0x0000000000000318 - 0x0000000000000334 is .interp
	0x0000000000000338 - 0x0000000000000368 is .note.gnu.property
	0x0000000000000368 - 0x000000000000038c is .note.gnu.build-id
	0x000000000000038c - 0x00000000000003ac is .note.ABI-tag
	0x00000000000003b0 - 0x00000000000003d4 is .gnu.hash
	0x00000000000003d8 - 0x0000000000000498 is .dynsym
	0x0000000000000498 - 0x000000000000052e is .dynstr
	0x000000000000052e - 0x000000000000053e is .gnu.version
	0x0000000000000540 - 0x0000000000000570 is .gnu.version_r
	0x0000000000000570 - 0x0000000000000630 is .rela.dyn
	0x0000000000000630 - 0x0000000000000660 is .rela.plt
	0x0000000000001000 - 0x000000000000101b is .init
	0x0000000000001020 - 0x0000000000001050 is .plt
	0x0000000000001050 - 0x0000000000001060 is .plt.got
	0x0000000000001060 - 0x0000000000001080 is .plt.sec
	0x0000000000001080 - 0x00000000000011d2 is .text
	0x00000000000011d4 - 0x00000000000011e1 is .fini
	0x0000000000002000 - 0x0000000000002042 is .rodata
	0x0000000000002044 - 0x0000000000002078 is .eh_frame_hdr
	0x0000000000002078 - 0x0000000000002124 is .eh_frame
	0x0000000000003db0 - 0x0000000000003db8 is .init_array
	0x0000000000003db8 - 0x0000000000003dc0 is .fini_array
	0x0000000000003dc0 - 0x0000000000003fb0 is .dynamic
	0x0000000000003fb0 - 0x0000000000004000 is .got
	0x0000000000004000 - 0x0000000000004010 is .data
	0x0000000000004010 - 0x0000000000004018 is .bss
```


Y aquí tenemos el desensamblado del main:

```text
(gdb) disass main 
Dump of assembler code for function main:
   0x0000000000001169 <+0>:     endbr64
   0x000000000000116d <+4>:	    push   rbp
   0x000000000000116e <+5>: 	mov    rbp,rsp
   0x0000000000001171 <+8>:	    sub    rsp,0x10
   0x0000000000001175 <+12>:	lea    rax,[rip+0xe88]        # 0x2004
   0x000000000000117c <+19>:	mov    QWORD PTR [rbp-0x8],rax
   0x0000000000001180 <+23>:	mov    rax,QWORD PTR [rbp-0x8]
   0x0000000000001184 <+27>:	lea    rdx,[rip+0xe85]        # 0x2010
   0x000000000000118b <+34>:	mov    rsi,rdx
   0x000000000000118e <+37>:	mov    rdi,rax
   0x0000000000001191 <+40>:	call   0x1070 <strcmp@plt>
   0x0000000000001196 <+45>:	test   eax,eax
   0x0000000000001198 <+47>:	jne    0x11b7 <main+78>
   0x000000000000119a <+49>:	mov    rax,QWORD PTR [rbp-0x8]
   0x000000000000119e <+53>:	mov    rsi,rax
   0x00000000000011a1 <+56>:	lea    rax,[rip+0xe74]        # 0x201c
   0x00000000000011a8 <+63>:	mov    rdi,rax
   0x00000000000011ab <+66>:	mov    eax,0x0
   0x00000000000011b0 <+71>:	call   0x1060 <printf@plt>
   0x00000000000011b5 <+76>:	jmp    0x11cb <main+98>
   0x00000000000011b7 <+78>:	lea    rax,[rip+0xe6d]        # 0x202b
   0x00000000000011be <+85>:	mov    rdi,rax
   0x00000000000011c1 <+88>:	mov    eax,0x0
   0x00000000000011c6 <+93>:	call   0x1060 <printf@plt>
   0x00000000000011cb <+98>:	mov    eax,0x0
   0x00000000000011d0 <+103>:	leave
   0x00000000000011d1 <+104>:	ret
End of assembler dump.
```

Vamos a seguir el mismo método que en el anterior y veamos qué contiene cada dirección de memoria cargada en rax mediante la instrucción lea (todas en .rodata):

```text
(gdb) x/s 0x2004
0x2004:	"01234567891"
(gdb) x/s 0x2010
0x2010:	"Jsfdb56d65a"
(gdb) x/s 0x201c
0x201c:	"%s is password"
(gdb) x/s 0x202b
0x202b:	"Look at it as cracker "
```

Antes de poner una explicación sobre lo que podemos incluso llegar a deducir de aquí, hay algo que no había comentado y merece la pena destacar. Esto es lo que sucede cuando ejecutamos el binario: 

```bash
./level2 
Look at it as cracker 
```

Se cierra directamente con este mensaje. 

Ahora bien, dicho esto, ¿qué podemos deducir del output anterior a este? Pues básicamente, sin mirar ensamblador, podemos entender con total claridad que se comparan con la función strcmp dos cadenas de texto hardcodeadas, por lo que tenemos total certeza de que el programa nunca va a ejecutar el camino correcto, porque nunca pueden ser iguales. 

Dicho esto, aún con esta deducción, podemos comprobar (y debemos) nuestra hipótesis para ver si es certera o no, y ver si el Assembly nos da la razón. 

Siendo así, y para resumir, en este caso debemos identificar las instrucciónes que nos interesan como objetivo de ataque, que son estas:

```text
   0x0000000000001196 <+45>:	test   eax,eax
   0x0000000000001198 <+47>:	jne    0x11b7 <main+78>
```

Sabemos que strcmp recibe dos argumentos, así que vemos en las instrucciones del main+12 a main+37 cómo se está preparando el terreno para llamar a strcmp. Luego de llamarla, se compara si lo que devuelve es igual a 0 (comprobación de si es verdadero) con el test. Si no es igual (el jne), nos lleva al main+78, que sabemos qué contiene "look at it as cracker" para posteriormente seguir el flujo de las instrucciones y cerrarse.

Mirad, mi primer enfoque fue comparar los opcodes en el volcado estático con objdump para poder ver si podía identificarlos correctamente (eso es lo que, considero, tiene que hacer alguien que quiera apañárselas). Mi primer paso fue intentar cambiar la instrucción entera por un xor. test eax,eax y xor eax,eax equivalen a dos bytes, por lo que la longitud en este caso coincidiría y habría funcionado.

Aquí hay un matiz que no he explicado en ningún writeup y quiero dejarlo escrito. xor es una instrucción que compara a nivel de bits los ceros y unos dejando en 0 los bits que sean iguales y en 1 los que sean diferentes, por lo que comparar un valor contra sí mismo lo que hace es dejarlo a 0. Dicho esto, xor eax,eax hubiera destruido el valor real de strcmp dejando eax siempre a 0, por lo que el programa entraría siempre en la rama de éxito. Aunque funcionaría para conseguir el acceso, es menos elegante que la que veremos.

La que vamos a ver es más elegante porque respeta la lógica y solo invierte la decisión final. Lo que vamos a hacer nosotros es cambiar el opcode (la instrucción por un je), por lo que sólo llegaría a la parte del error en caso de ser iguales los strings. Misma idea, diferente lógica, válida igualmente.

Estos son los opcodes de je y jne:

    · je: 0x74 + un byte de desplazamiento

    · jne: 0x75 + un byte de desplazamiento

Ahora sí, sabiendo esto, procedamos al cambio a nivel de bytes con el editor hex (que está claro que me encanta por estos dos writeups en uno y el anterior de u-cant-pass jaja).

Primero, identificamos la secuencia que nos interesa:

```bash
objdump -d -M intel level2 | grep -C1 "1198"
    1196:	85 c0                	test   eax,eax
    1198:	75 1d                   jne    11b7 <main+0x4e>
            ^^
            

    119a:	48 8b 45 f8          	mov    rax,QWORD PTR [rbp-0x8]
```

Ahora sí, abrimos el binario con el editor de texto y lo convertimos (una vez dentro) a vista hexadecimal con xxd con el comando `nvim -b level2_patched` y luego (dentro) `:%!xxd`.

Una vez dentro, filtramos y encontramos esta línea:

```text
00001190: c7e8 dafe ffff 85c0 751d 488b 45f8 4889  ........u.H.E.H.
                              ^--^

```

Teniendo la secuencia identificada, cambiamos el byte 75 que corresponde al primer byte del opcode de jne por el byte del opcode de je, es decir, 75 por 74.

Ahora sí, procedemos a guardar el cambio primero revirtiéndolo con `:%!xxd -r` y aplicamos los cambios con `:wq` (nvim).

Al ejecutar ahora el binario, obtenemos esto:

```bash
./level2_patched 
01234567891 is password
```

## Conclusión

Aunque estos binarios son sencillos, quería traerlos para mostrar un enfoque distinto al analizar (cómo reconocer patrones rápidamente y decidir el punto de intervención mínimo). Están en `binary-analysis` en vez de `crackmes` por ese motivo. El foco es el método o la aproximación, no el reto en sí, simplemente me pareció interesante. 

De cara al futuro (aunque seguramente traiga bastantes otros crackmes antes), me gustaría analizar mi primer proyecto, el de fstab, a nivel de binario, para ver qué encuentro. Será un análisis exhaustivo, y creo que puede enseñarme muchísimo.

Por el momento, quiero seguir usando crackmes beginner friendly como entrenamiento real antes de pasar a técnicas de ofuscación, desbordamientos de pila, evasión de canaries, o modificación precisa de direcciones de retorno. Son un escalón más complejo (aunque existan crackmes sencillos sobre estos temas) y prefiero adquirir mejores cimientos primero. Es probable que traiga este tipo de contenido una vez tenga resueltos más de 20-30 crackmes sencillos (aunque no suba todos los writeups).

