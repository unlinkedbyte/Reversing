### Primer crackme documentado en el repositorio: "U Can't Pass" de mystergaif

Buenas. Hoy vamos a hacer un writeup sobre el crackme mencionado en el título, de la plataforma crackmes.one . He hecho algunos otros entre ayer y hoy (beginner friendly) pero este me ha llamado la atención por un motivo que hoy vamos a poner en práctica: modificar los bytes con un editor hex. Esta idea no surge de la nada, curiosamente estaba hoy viendo un directo de gynvael coldwin sobre ingenieria inversa, y la diferencia de nivel es tan abismal que asusta. Pero él, en ese directo, justamente jugaba a nivel de bytes con el binario, y creo que lo podemos extrapolar a este (se puede solucionar sin, pero ya verás que está chulo). 

Aquí tienes la URL por si quisieras descargarlo: [U-Can't-Pass](https://crackmes.one/crackme/69cd36983e328e778db052c2)


## Estructura

Empezamos, como siempre, con el pre-análisis del binario, para ver qué información obtenemos.

1. **file <binario>**

```bash
main.out: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 3.2.0, not stripped
```

Vamos a hacer un repaso de cada campo:

`ELF` nos indica el formato del binario, típico de sistemas Unix. Es un binario de 64 bits, LSB (least significant byte first) nos indica que es little-endian. Luego, pie executable: pie es la propiedad del binario que permite que el ASLR se ejecute. Vemos la arquitectura (x86-64) y luego la ABI (SYSV). Vemos que está enlazado dinámicamente y su intérprete al lado. Además, vemos que no está stripped (conserva símbolos y los nombres de las funciones).


2. **strings <binario>**

```bash
/lib64/ld-linux-x86-64.so.2
__libc_start_main
__cxa_finalize
printf
libc.so.6
GLIBC_2.2.5
GLIBC_2.34
_ITM_deregisterTMCloneTable
__gmon_start__
_ITM_registerTMCloneTable
PTE1
u+UH
Hello in my first programm for crackme.one
Success!
Error!
9*3$"
GCC: (Gentoo 15.2.1_p20251122 p3) 15.2.1 20251122
GCC: (Gentoo 15.2.1_p20260214 p5) 15.2.1 20260214
clang version 21.1.8
main.c
_DYNAMIC
__GNU_EH_FRAME_HDR
_GLOBAL_OFFSET_TABLE_
__libc_start_main@GLIBC_2.34
_ITM_deregisterTMCloneTable
_edata
_fini
printf@GLIBC_2.2.5
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
.symtab
.strtab
.shstrtab
.interp
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
.note.gnu.property
.note.ABI-tag
.init_array
.fini_array
.dynamic
.data
.bss
.comment
```

No te asustes por el output (si quieres una explicación más exhaustiva de estos campos, puedes ir a los dos primeros writeups de `binary-analysis`. En el primero hay algunos errores conceptuales que el segundo no tiene, aun así, hay bastante lógica que hoy probablemente me salte). Podemos ver funciones propias de glibc, el compilador con su versión, las secciones... En este caso, si dejamos el ruido aparente de lado, podemos ver strings hardcoded en el propio programa y la función printf:

```text
Hello in my first programm for crackme.one
Success!
Error!
```

No parece que haya funciones auxiliares, por lo que esto sugiere que toda la lógica del programa vive dentro del main (lo confirmaremos al entrar en el análisis estático). 

3. **readelf <flags> <binario>**

```bash
readelf -h main.out

Encabezado ELF:
  Mágico:  7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Clase:                             ELF64
  Datos:                             complemento a 2, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  Versión ABI:                       0
  Tipo:                              DYN (Position-Independent Executable file)
  Máquina:                           Advanced Micro Devices X86-64
  Versión:                           0x1
  Dirección del punto de entrada:    0x1060
  Inicio de encabezados de programa: 64  (bytes en el fichero)
  Inicio de encabezados de sección:  13568 (bytes en el fichero)
  Opciones:                          0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         13
  Size of section headers:           64 (bytes)
  Number of section headers:         30
  Section header string table index: 29

```

Aquí vemos el, básicamente, el comando file pero más exhaustivo. Vemos los bytes mágicos de la cabecera ELF (los fijos llegan hasta el 46, los 3 siguientes son variables y nos dan información sobre el mismo, mientras que los demás que están a cero todavía no los he investigado. Los 3 que menciono variables, también los expliqué en el primer ejercicio de binary-analysis). Como vemos, nos confirma en gran parte lo visto con el comando `file`. Cabe destacar que, las otras dos flags de las cuales no pondré hoy el output, son importantes. Hoy, por el bien de la simplicidad al ser un binario pequeño, las vamos a omitir. Las flags son `-l` y `-S`, por si alguien quisiera investigarlas.

Dicho esto, vamos a pasar al análisis estático después de mapear el terreno.

## Análisis estático

Como acabo de decir, por el bien de la simplicidad, hoy no abriremos un descompilador, pasaremos al análisis estático con objdump para posteriormente ir a gdb. Este es el output con su comando: 

```bash
objdump -d -M intel main.out

main.out:     formato del fichero elf64-x86-64


Desensamblado de la sección .init:

0000000000001000 <_init>:
    1000:	f3 0f 1e fa          	endbr64
    1004:	48 83 ec 08          	sub    rsp,0x8
    1008:	48 8b 05 d9 2f 00 00 	mov    rax,QWORD PTR [rip+0x2fd9]        # 3fe8 <__gmon_start__>
    100f:	48 85 c0             	test   rax,rax
    1012:	74 02                	je     1016 <_init+0x16>
    1014:	ff d0                	call   rax
    1016:	48 83 c4 08          	add    rsp,0x8
    101a:	c3                   	ret

Desensamblado de la sección .plt:

0000000000001020 <.plt>:
    1020:	ff 35 9a 2f 00 00    	push   QWORD PTR [rip+0x2f9a]        # 3fc0 <_GLOBAL_OFFSET_TABLE_+0x8>
    1026:	ff 25 9c 2f 00 00    	jmp    QWORD PTR [rip+0x2f9c]        # 3fc8 <_GLOBAL_OFFSET_TABLE_+0x10>
    102c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
    1030:	f3 0f 1e fa          	endbr64
    1034:	68 00 00 00 00       	push   0x0
    1039:	e9 e2 ff ff ff       	jmp    1020 <_init+0x20>
    103e:	66 90                	xchg   ax,ax

Desensamblado de la sección .plt.got:

0000000000001040 <__cxa_finalize@plt>:
    1040:	f3 0f 1e fa          	endbr64
    1044:	ff 25 ae 2f 00 00    	jmp    QWORD PTR [rip+0x2fae]        # 3ff8 <__cxa_finalize@GLIBC_2.2.5>
    104a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

Desensamblado de la sección .plt.sec:

0000000000001050 <printf@plt>:
    1050:	f3 0f 1e fa          	endbr64
    1054:	ff 25 76 2f 00 00    	jmp    QWORD PTR [rip+0x2f76]        # 3fd0 <printf@GLIBC_2.2.5>
    105a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

Desensamblado de la sección .text:

0000000000001060 <_start>:
    1060:	f3 0f 1e fa          	endbr64
    1064:	31 ed                	xor    ebp,ebp
    1066:	49 89 d1             	mov    r9,rdx
    1069:	5e                   	pop    rsi
    106a:	48 89 e2             	mov    rdx,rsp
    106d:	48 83 e4 f0          	and    rsp,0xfffffffffffffff0
    1071:	50                   	push   rax
    1072:	54                   	push   rsp
    1073:	45 31 c0             	xor    r8d,r8d
    1076:	31 c9                	xor    ecx,ecx
    1078:	48 8d 3d d1 00 00 00 	lea    rdi,[rip+0xd1]        # 1150 <main>
    107f:	ff 15 53 2f 00 00    	call   QWORD PTR [rip+0x2f53]        # 3fd8 <__libc_start_main@GLIBC_2.34>
    1085:	f4                   	hlt
    1086:	66 2e 0f 1f 84 00 00 	cs nop WORD PTR [rax+rax*1+0x0]
    108d:	00 00 00 
    1090:	48 8d 3d 79 2f 00 00 	lea    rdi,[rip+0x2f79]        # 4010 <__TMC_END__>
    1097:	48 8d 05 72 2f 00 00 	lea    rax,[rip+0x2f72]        # 4010 <__TMC_END__>
    109e:	48 39 f8             	cmp    rax,rdi
    10a1:	74 15                	je     10b8 <_start+0x58>
    10a3:	48 8b 05 36 2f 00 00 	mov    rax,QWORD PTR [rip+0x2f36]        # 3fe0 <_ITM_deregisterTMCloneTable>
    10aa:	48 85 c0             	test   rax,rax
    10ad:	74 09                	je     10b8 <_start+0x58>
    10af:	ff e0                	jmp    rax
    10b1:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
    10b8:	c3                   	ret
    10b9:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
    10c0:	48 8d 3d 49 2f 00 00 	lea    rdi,[rip+0x2f49]        # 4010 <__TMC_END__>
    10c7:	48 8d 35 42 2f 00 00 	lea    rsi,[rip+0x2f42]        # 4010 <__TMC_END__>
    10ce:	48 29 fe             	sub    rsi,rdi
    10d1:	48 89 f0             	mov    rax,rsi
    10d4:	48 c1 ee 3f          	shr    rsi,0x3f
    10d8:	48 c1 f8 03          	sar    rax,0x3
    10dc:	48 01 c6             	add    rsi,rax
    10df:	48 d1 fe             	sar    rsi,1
    10e2:	74 14                	je     10f8 <_start+0x98>
    10e4:	48 8b 05 05 2f 00 00 	mov    rax,QWORD PTR [rip+0x2f05]        # 3ff0 <_ITM_registerTMCloneTable>
    10eb:	48 85 c0             	test   rax,rax
    10ee:	74 08                	je     10f8 <_start+0x98>
    10f0:	ff e0                	jmp    rax
    10f2:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    10f8:	c3                   	ret
    10f9:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
    1100:	f3 0f 1e fa          	endbr64
    1104:	80 3d 05 2f 00 00 00 	cmp    BYTE PTR [rip+0x2f05],0x0        # 4010 <__TMC_END__>
    110b:	75 2b                	jne    1138 <_start+0xd8>
    110d:	55                   	push   rbp
    110e:	48 83 3d e2 2e 00 00 	cmp    QWORD PTR [rip+0x2ee2],0x0        # 3ff8 <__cxa_finalize@GLIBC_2.2.5>
    1115:	00 
    1116:	48 89 e5             	mov    rbp,rsp
    1119:	74 0c                	je     1127 <_start+0xc7>
    111b:	48 8b 3d e6 2e 00 00 	mov    rdi,QWORD PTR [rip+0x2ee6]        # 4008 <__dso_handle>
    1122:	e8 19 ff ff ff       	call   1040 <__cxa_finalize@plt>
    1127:	e8 64 ff ff ff       	call   1090 <_start+0x30>
    112c:	c6 05 dd 2e 00 00 01 	mov    BYTE PTR [rip+0x2edd],0x1        # 4010 <__TMC_END__>
    1133:	5d                   	pop    rbp
    1134:	c3                   	ret
    1135:	0f 1f 00             	nop    DWORD PTR [rax]
    1138:	c3                   	ret
    1139:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
    1140:	f3 0f 1e fa          	endbr64
    1144:	e9 77 ff ff ff       	jmp    10c0 <_start+0x60>
    1149:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]

0000000000001150 <main>:
    1150:	f3 0f 1e fa          	endbr64
    1154:	55                   	push   rbp
    1155:	48 89 e5             	mov    rbp,rsp
    1158:	48 83 ec 10          	sub    rsp,0x10
    115c:	c7 45 fc 00 00 00 00 	mov    DWORD PTR [rbp-0x4],0x0
    1163:	48 8d 3d 9a 0e 00 00 	lea    rdi,[rip+0xe9a]        # 2004 <_IO_stdin_used+0x4>
    116a:	b0 00                	mov    al,0x0
    116c:	e8 df fe ff ff       	call   1050 <printf@plt>
    1171:	c7 45 f8 0a 00 00 00 	mov    DWORD PTR [rbp-0x8],0xa
    1178:	83 7d f8 0a          	cmp    DWORD PTR [rbp-0x8],0xa
    117c:	74 10                	je     118e <main+0x3e>
    117e:	48 8d 3d ab 0e 00 00 	lea    rdi,[rip+0xeab]        # 2030 <_IO_stdin_used+0x30>
    1185:	b0 00                	mov    al,0x0
    1187:	e8 c4 fe ff ff       	call   1050 <printf@plt>
    118c:	eb 16                	jmp    11a4 <main+0x54>
    118e:	83 7d f8 0a          	cmp    DWORD PTR [rbp-0x8],0xa
    1192:	75 0e                	jne    11a2 <main+0x52>
    1194:	48 8d 3d 9f 0e 00 00 	lea    rdi,[rip+0xe9f]        # 203a <_IO_stdin_used+0x3a>
    119b:	b0 00                	mov    al,0x0
    119d:	e8 ae fe ff ff       	call   1050 <printf@plt>
    11a2:	eb 00                	jmp    11a4 <main+0x54>
    11a4:	8b 45 fc             	mov    eax,DWORD PTR [rbp-0x4]
    11a7:	48 83 c4 10          	add    rsp,0x10
    11ab:	5d                   	pop    rbp
    11ac:	c3                   	ret

Desensamblado de la sección .fini:

00000000000011b0 <_fini>:
    11b0:	f3 0f 1e fa          	endbr64
    11b4:	48 83 ec 08          	sub    rsp,0x8
    11b8:	48 83 c4 08          	add    rsp,0x8
    11bc:	c3                   	ret

```

No te preocupes por toda la cantidad de información. Lo que vemos aquí cubre todas las secciones que contienen código ejecutable. A modo de introducción, veamos que se carga en cada una:

    · `.init`: Es la sección que pertenece al código que se ejecuta muy al principio, antes de que main arranque (inicialización básicamente).

    · `.plt/.plt.got/.plt.sec`: son los "saltos" hacia funciones de librerías dinámicas.

    · `.text`: Aquí vive el código real, el main que escribió el autor, y el resto de la infraestructura generada por el compilador (ej: frame_dummy, register_tm_clones..).

    ·`.fini`: El código de cierre para cuando el programa termina, "simétrico" a .init .

Estas secciones que se cargan en memoria forman parte del segmento LOAD que podrás ver con readelf -l (una de ellas)

*(Por cierto, un error que cometí en el primer writeup, que seguramente vuelva a comentar en el directorio dedicado al análisis de binarios, es que, si nos fijamos en donde empiezan las secciones en nuestro binario (por ejemplo, con `info file` en gdb), intentar leer información de .rodata (que es donde viven las cadenas de texto de nuestro código) como instrucción, te va a dar basura que gdb intenta interpretar. Error de novato jaja! Todavía hacen falta pulir muchas cosas).*

Vamos ya a por el análisis dinámico, que hoy sí que quiero explicar unas cuántas cosas importantes.

## Análisis dinámico

Como en nuestros anteriores análisis por el momento, vamos a abrir GDB, desensamblar el main.

Un matiz que quiero destacar para las posteriores explicaciones: Cuando veamos ahora (solo con el desensamblado) las direcciones de memoria virtuales a la izquierda, son offsets estáticos que vemos **antes** de ejecutar el programa, calculados como si el binario se cargara siempre en la dirección base 0x0. Pero como es PIE, el kernel le asigna una base aleatoria en cada ejecución. Lo comento porque no puedes poner un breakpoint si no esta corriendo sobre una dirección numérica literal contra un binario PIE, te pondrá que no puedes acceder a esa parte de la memoria, igual luego pongo un ejemplo práctico. Dicho esto, vamos a desensamblar el main para averiguar como funciona el binario por detras o cómo era el código fuente. 

Antes de nada, quiero mostraros un comando útil de gdb que nos indica donde estan las secciones:

```text
info file   // -> el comando al que me refería
Symbols from "/home/ygm/crackmes/u-cant-pass/main.out".
Local exec file:
	`/home/ygm/crackmes/u-cant-pass/main.out', file type elf64-x86-64.
	Entry point: 0x1060
	0x0000000000000318 - 0x0000000000000334 is .interp
	0x0000000000000338 - 0x000000000000035c is .gnu.hash
	0x0000000000000360 - 0x0000000000000408 is .dynsym
	0x0000000000000408 - 0x0000000000000497 is .dynstr
	0x0000000000000498 - 0x00000000000004a6 is .gnu.version
	0x00000000000004a8 - 0x00000000000004d8 is .gnu.version_r
	0x00000000000004d8 - 0x0000000000000598 is .rela.dyn
	0x0000000000000598 - 0x00000000000005b0 is .rela.plt
	0x0000000000001000 - 0x000000000000101b is .init
	0x0000000000001020 - 0x0000000000001040 is .plt
	0x0000000000001040 - 0x0000000000001050 is .plt.got
	0x0000000000001050 - 0x0000000000001060 is .plt.sec
	0x0000000000001060 - 0x00000000000011ad is .text
	0x00000000000011b0 - 0x00000000000011bd is .fini
	0x0000000000002000 - 0x0000000000002041 is .rodata
	0x0000000000002044 - 0x0000000000002078 is .eh_frame_hdr
	0x0000000000002078 - 0x0000000000002124 is .eh_frame
	0x0000000000002128 - 0x0000000000002158 is .note.gnu.property
	0x0000000000002158 - 0x0000000000002178 is .note.ABI-tag
	0x0000000000003db8 - 0x0000000000003dc0 is .init_array
	0x0000000000003dc0 - 0x0000000000003dc8 is .fini_array
	0x0000000000003dc8 - 0x0000000000003fb8 is .dynamic
	0x0000000000003fb8 - 0x0000000000004000 is .got
	0x0000000000004000 - 0x0000000000004010 is .data
	0x0000000000004010 - 0x0000000000004018 is .bss
```

Ahora sí, el desensamblado del main:

```text
(gdb) disassemble main
Dump of assembler code for function main:
   0x0000000000001150 <+0>:	endbr64
   0x0000000000001154 <+4>:	push   rbp
   0x0000000000001155 <+5>:	mov    rbp,rsp
   0x0000000000001158 <+8>:	sub    rsp,0x10
   0x000000000000115c <+12>:	mov    DWORD PTR [rbp-0x4],0x0
   0x0000000000001163 <+19>:	lea    rdi,[rip+0xe9a]        # 0x2004
   0x000000000000116a <+26>:	mov    al,0x0
   0x000000000000116c <+28>:	call   0x1050 <printf@plt>
   0x0000000000001171 <+33>:	mov    DWORD PTR [rbp-0x8],0xa
   0x0000000000001178 <+40>:	cmp    DWORD PTR [rbp-0x8],0xa
   0x000000000000117c <+44>:	je     0x118e <main+62>
   0x000000000000117e <+46>:	lea    rdi,[rip+0xeab]        # 0x2030
   0x0000000000001185 <+53>:	mov    al,0x0
   0x0000000000001187 <+55>:	call   0x1050 <printf@plt>
   0x000000000000118c <+60>:	jmp    0x11a4 <main+84>
   0x000000000000118e <+62>:	cmp    DWORD PTR [rbp-0x8],0xa
   0x0000000000001192 <+66>:	jne    0x11a2 <main+82>
   0x0000000000001194 <+68>:	lea    rdi,[rip+0xe9f]        # 0x203a
   0x000000000000119b <+75>:	mov    al,0x0
   0x000000000000119d <+77>:	call   0x1050 <printf@plt>
   0x00000000000011a2 <+82>:	jmp    0x11a4 <main+84>
   0x00000000000011a4 <+84>:	mov    eax,DWORD PTR [rbp-0x4]
   0x00000000000011a7 <+87>:	add    rsp,0x10
   0x00000000000011ab <+91>:	pop    rbp
   0x00000000000011ac <+92>:	ret
End of assembler dump.
```

Bueno, procedamos. Como esta es la primera vez que veo esa instrucción (endbr64), vamos a investigarla rápidamente:

    · `endbr64`: Es una instrucción de seguridad relativamente reciente del procesador, parte de una tecnología Intel llamada CET (Control-flow Enforcement Technology). Su función es marcar explícitamente "este punto es un destino válido para un salto o llamada indirecta". Por qué existe y qué problema resuelve? Por la familia de ataques ROP/JOP (Return/Jump Oriented Programming), donde un atacante, aprovechando un bug de memoria, hace que la ejecución salte a mitad de una función existente en vez de por su flujo de entrada normal, encadenando fragmentos de código para construir un ataque completo sin inyectar código nuevo. Esta instrucción lo que hace es que, cuando la CPU ejecuta un salto o llamada indirecta (a través de un registro o puntero, no a una dirección fija escrita en el código), comprueba que el destino empiece exactamente con una instrucción endbr64. Si no la encuentra ahí, genera una excepción y el programa se detiene (porque eso indica que el salto está aterrizando en mitad de una función, no en su entrada).

Dicho esto, y ahora sí, empecemos con el análisis. 

Como siempre, el prólogo, que esta vez son 4 instrucciones. La siguiente de endbr64 es push rbp. Como ya sabemos, push rbp (esto es un base pointer) lo que hace es guardar el valor actual del registro. Guarda el frame pointer de la función que ha llamado (en este caso) al main. Lo guardamos porque vamos a reescribirlo creando otro anclaje en la siguiente instrucción, que nos permitirá movernos por la función con las direcciones relativas pertinentes. Al salir, se restaura con el último pop rbp que vemos en el final del desensamblado. 

Creado el nuevo punto de anclaje del qué guiarse el programa, ahora (con sub) resta 0x10 a rsp. Por qué se hace esto? esta reservando la memoria que la propia función del main va a usar. Cuando hacemos la conversión, vemos que hemos reservado 16 bytes (0x10 = 16). Con esto, ha finalizado el prólogo y ya tenemos el stack frame creado. 

Para que sea más didáctico para el análisis, vamos a poner de 6 en 6 instrucciones:

```text
   0x000000000000115c <+12>:	mov    DWORD PTR [rbp-0x4],0x0
   0x0000000000001163 <+19>:	lea    rdi,[rip+0xe9a]        # 0x2004
   0x000000000000116a <+26>:	mov    al,0x0
   0x000000000000116c <+28>:	call   0x1050 <printf@plt>
   0x0000000000001171 <+33>:	mov    DWORD PTR [rbp-0x8],0xa
   0x0000000000001178 <+40>:	cmp    DWORD PTR [rbp-0x8],0xa
   0x000000000000117c <+44>:	je     0x118e <main+62>
```

Antes de nada, veis que hay 3 instrucciones lea con la dirección calculada a la derecha que pertenece a .rodata (confirmado por el info file del principio)? Vamos a comprobar qué contienen cada una, para salir ya de dudas:

```text
(gdb) x/s 0x2004
0x2004:	"Hello in my first programm for crackme.one\n"
(gdb) x/s 0x2030
0x2030:	"Success!\n"
(gdb) x/s 0x203a
0x203a:	"Error!"
```

Así, ya nos hacemos una idea de qué nos podemos encontrar cerca. 

Volviendo a las 6 instrucciones de arriba:

La primera línea es el típico patrón que nos indica que se ha inicializado una variable a 0, llamémosla 'i'. Sabemos que es un int porque la operación mueve 4 bytes (dword -> double word = leer/escribir 4 bytes en esa dirección de memoria), que es exactamente lo que ocupa un int en un sistema de 64 bits. 

Posteriormente se carga la dirección 0xe9a relativa al rip (instruction pointer) que es 0x2004, ya calculada por gdb, en rdi. `rdi` es un registro que habitualmente se usa para el primer argumento de una función (aunque, debo decir, que he descubierto que los registros son más flexibles de lo que me pensaba). Como vemos el printf sin estar optimizado por el compilador como puts, podríamos deducir que las cadenas que imprime tienen un especificador de formato que reciben un argumento, por ejemplo. Ahora bien, no parece ser este caso y no quiero asegurar si siempre es así, quizá tenga que ver también en la versión o qué compilador se ha usado. Dicho esto, hemos guardado en rdi esa dirección de memoria (que sabemos que contiene "Hello in my first program for crackme.one\n". 

Por qué esta vez no parece ser que reciba un especificador de formato o argumento la función printf? porque cuando ejecutamos este binario, se cierra directamente con el tercer mensaje que vemos cuando hemos analizado lo de .rodata: "Error!", y termina directamente. 

Dicho esto, luego, copiamos un 0x0 a al. Cuando vemos por ejemplo 'mov   eax, 0x0' antes de una función como printf, justamente nos indica que recibe un argumento, y que no es un flotante. Profundizaré más en esto en otros crackmes que traiga, que me estoy desviando. Para ser honesto, no tengo certeza si es aplicable ese patrón aquí, así que no quiero asegurar que el registro al lo vayamos a usar para esto. Lo dejo en el aire para otro momento. 

Luego, estamos copiando 0xa a rbp-0x8. Aquí estamos desreferenciando, por lo que ahora, la dirección que apunta a rbp-0x8, contiene 10 (0xa es 10 en decimal). Posteriormente realizamos un compare con cmp, compara si rbp-0x8 contiene el valor 10, lo cual es así porque se lo acaba de pasar. Esto puede parecer una redundancia porque sabemos que se va a cumplir automáticamente, pero ese es el qué de este crackme. 

Ahora, la siguiente instrucción es je (jump if equal) a main + 62. Vamos a analizar el flujo del programa desde el main + 62 hasta el final para ver que pasa ya que esta condición es verdadera:

```text
   0x000000000000118e <+62>:	cmp    DWORD PTR [rbp-0x8],0xa
   0x0000000000001192 <+66>:	jne    0x11a2 <main+82>
   0x0000000000001194 <+68>:	lea    rdi,[rip+0xe9f]        # 0x203a
   0x000000000000119b <+75>:	mov    al,0x0
   0x000000000000119d <+77>:	call   0x1050 <printf@plt>
   0x00000000000011a2 <+82>:	jmp    0x11a4 <main+84>
   0x00000000000011a4 <+84>:	mov    eax,DWORD PTR [rbp-0x4]
   0x00000000000011a7 <+87>:	add    rsp,0x10
   0x00000000000011ab <+91>:	pop    rbp
   0x00000000000011ac <+92>:	ret
```

En caso de ser igual (y lo es), nos lleva a otra comparación, donde vuelve a comprobar si rbp-0x8 contiene 10(0xa), lo que es correcto. Ahora, eso si, tenemos un jne (jump if not equal) a main + 82. Como sí que es igual, llegamos a la instrucción de lea, que, como sabemos, es una instrucción que copia la dirección de memoria de la fuente al destino. Como hemos puesto antes, 0x203a contiene "Error!". Es decir, este programa a simple vista parece que nos obliga a que nunca haya un success (como en el segundo lea visto anteriormente). Nos impone que siempre vaya al error, lo cual esto explica lo que sucede al ejecutarlo. Por cierto, rbp-0x8 simplemente es una variable de tipo int, llamémosla c, que inicializamos a 10 en main+33. Comento esto porque la estructura interna del código simplemente nos podría estar diciendo algo como "if (c == 10) {printf("Error"!) return 0;} else {printf("Success\n"); return 0;}". Luego, volvemos a ver el mismo patron de al, 0x0 del que no tengo certeza absoluta pero ocurre antes del printf (que imprime el error). 

Después del printf, hay un salto incondicional, que debemos seguir. Aunque es en la siguiente línea jaja. Luego, copiamos el valor de rbp-0x4 a eax (que contenia 0, era la variable i = 0 del principio si recordamos). Los eax al final de la función suelen indicar ese return 0, así que cuadra. Para luego, añadir 0x10 a rsp (16, lo mismo que al principio), para luego sacar rbp del stack y retornar al primer push rbp para la función que nos llamó. Estas tres últimas instrucciones son el epílogo. 

Vamos a analizar las que nos faltan y entramos en cómo evadir o hacer que el binario haga lo que nosotros queremos, que es lo chulo de todo!

Nos faltaban estas instrucciones del medio: 

```text
   0x0000000000001178 <+40>:	cmp    DWORD PTR [rbp-0x8],0xa
   0x000000000000117c <+44>:	je     0x118e <main+62>
   0x000000000000117e <+46>:	lea    rdi,[rip+0xeab]        # 0x2030
   0x0000000000001185 <+53>:	mov    al,0x0
   0x0000000000001187 <+55>:	call   0x1050 <printf@plt>
   0x000000000000118c <+60>:	jmp    0x11a4 <main+84>
```

Aquí esta el corazón del "Success!". Si en la segunda instrucción que no se ejecuta, podríamos llegar ahí, cargaría la dirección del string con lea a rdi para luego volver llamar a printf y hacer un salto incondicional a main+84, donde entraríamos en el epílogo. 

Bueno, en los dos ejercicios del binary-analysis planteaba la pregunta sobre la dificultad y la capacidad de abstracción, donde añadía lo complicado que podía llegar a ser imaginarte el código fuente sin tenerlo, pero es que en eso se basa la ingeniería inversa (entre otras tantas cosas, pero su "principal" finalidad). Dicho esto, creo que no ha ido tan mal el crackme (teniendo en cuenta todas las lagunas que hay y que llevamos 2 semanas a fondo, lo podemos considerar un logro, ya que hemos podido entender dónde esta el problema, aunque fuera sencillo). 


## Bypass: manipulación en caliente y posterior edición a nivel de bytes

Empecemos por la evasión de la comprobación, dejando lo mejor (la edición con un editor hexadecimal) para el final. Aquí tenemos dos maneras. Empezaremos por la más sencilla y desglosaré la segunda.

1. **La evasión sencilla**
En este paso simplemente ponemos un breakpoint en main+44 (que es la instrucción anterior al je), corremos el programa con `run` y nos saltamos la instrucción para ir directamente a main+46, donde obtendremos lo que queremos y el programa saldrá exitosamente:

```text
(gdb) break *main+44
Breakpoint 1 at 0x117c
(gdb) run
Starting program: /home/ygm/crackmes/u-cant-pass/main.out 
+[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
Hello in my first programm for crackme.one

Breakpoint 1, 0x000055555555517c in main ()
(gdb) jump *main+46
Continuing at 0x55555555517e.
Success!
[Inferior 1 (process 130695) exited normally]
```

Como podemos ver, nos hemos saltado el salto que hacía que siempre se cerrara el programa con un "Error!". 

Aquí debo explicar una cosa que tiene que ver con el ASLR: no puedes poner un breakpoint en 0x117c como tal, porque cuando el programa corra, las direcciones habrán cambiado: 

```text
(gdb) disassemble main 
Dump of assembler code for function main:
   0x0000555555555150 <+0>:	endbr64
   0x0000555555555154 <+4>:	push   rbp
   0x0000555555555155 <+5>:	mov    rbp,rsp
   0x0000555555555158 <+8>:	sub    rsp,0x10
   0x000055555555515c <+12>:	mov    DWORD PTR [rbp-0x4],0x0
   0x0000555555555163 <+19>:	lea    rdi,[rip+0xe9a]        # 0x555555556004
   0x000055555555516a <+26>:	mov    al,0x0
   0x000055555555516c <+28>:	call   0x555555555050 <printf@plt>
   0x0000555555555171 <+33>:	mov    DWORD PTR [rbp-0x8],0xa
   0x0000555555555178 <+40>:	cmp    DWORD PTR [rbp-0x8],0xa
   0x000055555555517c <+44>:	je     0x55555555518e <main+62>
   0x000055555555517e <+46>:	lea    rdi,[rip+0xeab]        # 0x555555556030
   0x0000555555555185 <+53>:	mov    al,0x0
   0x0000555555555187 <+55>:	call   0x555555555050 <printf@plt>
   0x000055555555518c <+60>:	jmp    0x5555555551a4 <main+84>
   0x000055555555518e <+62>:	cmp    DWORD PTR [rbp-0x8],0xa
   0x0000555555555192 <+66>:	jne    0x5555555551a2 <main+82>
   0x0000555555555194 <+68>:	lea    rdi,[rip+0xe9f]        # 0x55555555603a
   0x000055555555519b <+75>:	mov    al,0x0
   0x000055555555519d <+77>:	call   0x555555555050 <printf@plt>
   0x00005555555551a2 <+82>:	jmp    0x5555555551a4 <main+84>
   0x00005555555551a4 <+84>:	mov    eax,DWORD PTR [rbp-0x4]
   0x00005555555551a7 <+87>:	add    rsp,0x10
   0x00005555555551ab <+91>:	pop    rbp
   0x00005555555551ac <+92>:	ret
End of assembler dump.
```

**Qué ha ocurrido aquí?**

Fíjate en la diferencia entre el disassemble main de antes de ejecutar (0x0000000000001150) y este mismo comando, pero con el programa ya corriendo (0x0000555555555150). La instrucción es la misma, el offset relativo dentro de la función también (<+0>, <+4>...), lo único que cambió es la base.

Esto es exactamente lo que hace el PIE que identificamos con file al principio: al ser un ejecutable de posición independiente, el kernel le asigna una dirección base aleatoria cada vez que lo carga en memoria (esto es literalmente el ASLR en acción). Antes de correr, GDB/objdump solo pueden mostrarte el binario como si se cargara en la base 0x0, es la única referencia fija que existe sin un proceso real detrás. En cuanto ejecutas con run, el kernel ya ha elegido su base real (0x555555555000 en este caso concreto, distinta en cada ejecución), y todas las direcciones se recalculan sumando esa base al offset original.

Por eso break 0x117c (dirección literal, calculada sobre la base falsa 0x0) falla al insertar el breakpoint una vez el proceso está corriendo con otra base real. Esa dirección, tal cual, ya no corresponde a nada válido en el mapa de memoria real del proceso. En cambio, break main+44 funciona siempre, sea cual sea la base que le toque esa vez: GDB resuelve primero dónde está main en la dirección real del proceso ya cargado, y luego le suma el offset (+44) (la traducción la hace GDB por ti, en vez de tener que calcularla tú a mano cada vez).


2. **Modificando la variable en caliente**

Vamos directamente a la modificación. Esta se realiza en la comparación. Le modificamos el valor que contiene rbp-0x8 simplemente y "evadimos" la lógica del programa:

```text
(gdb) break *main+40
Breakpoint 1 at 0x1178
(gdb) run
Starting program: /home/ygm/crackmes/u-cant-pass/main.out 
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
Hello in my first programm for crackme.one

Breakpoint 1, 0x0000555555555178 in main ()
(gdb) set variable *(int *)($rbp-0x8) = 3
(gdb) cont
Continuing.
Success!
[Inferior 1 (process 131246) exited normally]
```

Debo añadir que, con el cansancio, sin querer había puesto el breakpoint en main+44, lo que me había mostrado esto el programa: 

```text
Breakpoint 1, 0x000055555555517c in main ()
(gdb) set variable *(int *)($rbp-0x8) = 3
(gdb) continue
Continuing.
[Inferior 1 (process 131162) exited normally]
```

Al no haber visto el Success, me había extrañado. Lo dejo aquí como pequeña anécdota, pero hay que poner el breakpoint en la comparación, sino la CPU ya tiene la decisión de saltar tomada en ese momento.

Lo que hacemos en la línea `set variable *(int *)($rbp-0x8) = 3` es desreferenciar(el * que está fuera de toda la operación) esa dirección tratándola como un puntero a un entero(int * ) , y le asignamos el nuevo valor.

3. **Modificación de bytes con editor hexadecimal**
Pasamos ahora a la modificación del byte con el editor hexadecimal. Este es el output del binario normal:

```bash
/main.out 
Hello in my first programm for crackme.one
Error!
```

Lo primero que vamos a hacer es localizar el offset exacto del byte con objdump (sobre el archivo estático):

```bash
$objdump -d -M intel main_patched.out | grep -C1 "1171:"
    116c:	e8 df fe ff ff       	call   1050 <printf@plt>
--> 1171:	c7 45 f8 0a 00 00 00 	mov    DWORD PTR [rbp-0x8],0xa   <----
    1178:	83 7d f8 0a          	cmp    DWORD PTR [rbp-0x8],0xa
```

Ya lo tengo confirmado por el propio análisis anterior, queremos cambiar el byte 0a, que es el 10. Los tres primeros bytes codifican la instrucción y su operando de destino ([rbp-0x8)]. 

Teniendo la copia ya hecha del binario a modificar, vamos a abrirlo con un editor de texto con el comando `nvim -b <nombre_binario>`. Una vez hecho esto, nos apareceran muchos data ilegible. Estando dentro, ejecutamos `:%!xxd` para verlo en hexadecimal. Como xxd corta los bloques en 16 bytes, no caerá lo que buscamos justamente en 0x1171. No pasa nada, vamos a buscar un bloque que sea cercano para ver con cual trabajamos: 

```text
00001160: 0000 0048 8d3d 9a0e 0000 b000 e8df feff  ...H.=..........                                   
00001170: ffc7 45f8 0a00 0000 837d f80a 7410 488d  ..E......}..t.H.  <-----                       
00001180: 3dab 0e00 00b0 00e8 c4fe ffff eb16 837d  =..............}    
```

Como podemos apreciar, el ff del principio corresponde a una instrucción anterior. Si comparamos el output de arriba, identificamos donde empieza y termina: `c7 45f8 0a00 0000`. Teniendo esto identificado correctamente, cambiamos el byte correspondiente al primer 0a de esa secuencia (a 03 por ejemplo) y debemos usar, despues de haberlo modificado, este comando: `:%!xxd -r` para luego guardar el archivo, con `:wq` (en mi caso, que ha sido en nvim).

Teniendolo ya editado, ahora, si ejecutamos el binario modificado, mirad el resultado:

```bash
./main_patched.out 
Hello in my first programm for crackme.one
Success!
```

¡Es una buena primera aproximación a los editores hexadecimales! ¡Este último paso, sin duda, es el que más adrenalina te da!

