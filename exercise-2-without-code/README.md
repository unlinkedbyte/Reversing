### Primeros pasos en reversing: destripando un binario del que no sabemos el código fuente

Para que este análisis fuera realmente a ciegas, le pedí a un LLM que generara un ejercicio corto en C sin enseñármelo - lo compilé directamente sin leer una sola línea del código fuente, copiándolo en el directorio sin mirarlo.

Es muy probable que este writeup sea muy parecido al primero. Seguiré las mismas pautas y veremos qué podemos averiguar. La duda que me surge es: ¿Cómo se entrena realmente esa capacidad de abstracción? 

Intentaré, eso si, que sea más pulido que el primero en la medida de lo posible. 

## Estructura

Como ya vimos en el primer writeup, esta es la estructura a seguir estándar: `file <binario> -> strings <binario> -> readelf (con sus flags) <binario> -> análisis estático -> análisis dinámico`. (Lo haré todo lo exhaustivo que se pueda sin aburrir al lector. Explicaré a muy malas todo lo que sea necesario. Pese a que no me gusta repetirme, si hace falta, se volverán a explicar conceptos que tocamos en el primer writeup).

1. **file <nombre_binario>**

Este es el output:

```bash
test: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=abdb860786f11749791f3331627f48d94928433d, for GNU/Linux 3.2.0, not stripped
```

Como podemos observar, empezamos con `ELF 64-bit`, que nos indica que es un binario en formato ELF de 64 bits, el formato estándar que usa Linux (y otros sistemas Unix) para sus ejecutables. 

'LSB' nos indica el orden de bytes que usa el binario, que en este caso es little endian (Least significan byte first). En las arquitecturas x86 siempre es little endian. Si escaneamos byte a byte alguna dirección de memoria legítima, veremos los bytes menos significativos delante, los primeros. 

'pie executable' es lo que hace posible que el ASLR se aplique al ejecutable. Hay una matiz que quiero recalcar, PIE es una propiedad del binario de cómo está compilado, mientras que ASLR es un mecanismo del kernel. El que decide la aleatorización del binario es el kernel y pie deja que se mueva a esa dirección sin que se rompa el binario. Si no tuviera PIE, el binario se compila asumiendo que siempre va a cargarse en una dirección base física fija. El PIE tiene que ver con lo que comentábamos ayer sobre el direccionamiento relativo del rip+offset. PIE -> position independent executable.

Otra cosa que no mencioné ayer y me parece importante como primera pasada es sobre el x86-64 que vemos después. x86 es una arquitectura de conjunto de instrucciones (ISA, instruction set architecture). Pese a que cuando pensamos en x86 lo primero que se nos pueda venir a la cabeza es el procesador de Intel, tiene más que ver con el conjunto de reglas de las instrucciones que existen. 64, como ya sabemos, es 64 bits. 

Otro matiz importante que no comenté ayer y hoy debo hacer con más exactitud es sobre la ABI. No es exactamente un intérprete. ABI (Application binary interface) es el conjunto de convenciones y reglas a las que un binario está ligado para poder ejecutarse e interactuar correctamente con el sistema operativo y con otros binarios/librerías en esa arquitectura concreta. 

Vemos que está enlazado dinámicamente, es decir, el binario no lleva incrustadas el código de las librerías que usa (como libc). Solo contiene referencias a esas funciones. El encargado de hacer esa conexión es el intérprete que vemos justo después. Luego vemos un hash, la versión y, para finalizar, si está stripped o no. Como bien dijimos ayer, un binario que esté stripped dificulta la lectura o el análisis por el simple hecho de que se ha eliminado la tabla de símbolos (los nombres de las funciones y variables), dejando solo direcciones numéricas. Si queremos ser más precisos, lo que desaparece es la traducción `dirección -> nombre`. Muchos binarios de producción se distribuyen así porque el usuario final no necesita esa información y permite ahorrar espacio, o para dificultar como acabo de mencionar el análisis de malware. 


2, **strings <nombre_binario>**

Con este comando podemos ver el texto que queda legible en el binario final (es decir, secuencias de bytes consecutivos que sean imprimibles como ASCII). Lo que vemos en el output son las secciones y los strings hardcodeados de los printf, que viven en la sección .rodata. Si el binario fuera stripped, strings funciona igual sobre los mensajes de .rodata porque no dependen de símbolos, sino de datos normales.

Aquí tenemos el output:

```bash
I(C=/lib64/ld-linux-x86-64.so.2
puts
exit
error
strlen
__libc_start_main
__cxa_finalize
libc.so.6
GLIBC_2.2.5
GLIBC_2.34
_ITM_deregisterTMCloneTable
__gmon_start__
_ITM_registerTMCloneTable
PTE1
u+UH
Usage: <binario> 4 digitos
Codigo correcto
Codigo incorrecto
;*3$"
GCC: (Debian 14.2.0-19) 14.2.0
Scrt1.o
__abi_tag
crtstuff.c
deregister_tm_clones
__do_global_dtors_aux
completed.0
__do_global_dtors_aux_fini_array_entry
frame_dummy
__frame_dummy_init_array_entry
ejercicio-sugerido.c
__FRAME_END__
_DYNAMIC
__GNU_EH_FRAME_HDR
_GLOBAL_OFFSET_TABLE_
__libc_start_main@GLIBC_2.34
_ITM_deregisterTMCloneTable
puts@GLIBC_2.2.5
_edata
error
_fini
strlen@GLIBC_2.2.5
__data_start
__gmon_start__
__dso_handle
_IO_stdin_used
_end
__bss_start
main
exit@GLIBC_2.2.5
__TMC_END__
_ITM_registerTMCloneTable
__cxa_finalize@GLIBC_2.2.5
_init
.symtab
.strtab
.shstrtab
.note.gnu.property
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
.plt.got
.text
.fini
.rodata
.eh_frame_hdr
.eh_frame
.note.ABI-tag
.init_array
.fini_array
.dynamic
.got.plt
.data
.bss
.comment
```

Como ya dijimos, hace falta mucha investigación que hacer todavía sobre las distintas secciones que habitan en un binario. Dicho esto, las que nos interesan a nosotros es .text, .rodata, .bss, .data.

Como podemos ver en el output, en las primeras filas, nos indica que usa la función puts, exit, strlen y error. Como error no la conozco o no me suena, pensaremos que hay una función aparte del main que sirva para el control de errores: 

```text
puts
exit
error
strlen
```

También podemos observar, como comentábamos antes, los strings hardcoded en el propio código. Podemos ver esto: 

```text
Usage: <binario> 4 digitos
Codigo correcto
Codigo incorrecto
```

Así, de primeras, poco más que añadir sobre el output de strings. Con esto último podemos observar que nos ha fabricado un ejercicio parecido a los que yo había hecho. Vemos que su modo de uso es introducir el nombre del binario y 4 dígitos como argumento. Como hemos visto strlen, podemos deducir (o más bien suponer) que habrá comprobaciones parecidas a las de ayer con `test eax, eax`, ya que deberá comprobar la longitud del argumento introducido o de una cadena de texto para posteriormente comprobar si es verdadero o no (== 0 || != 0). 

3. **readelf <flags> <nombre_binario>**

Ayer lo puse por ser buena práctica y por ser el estándar. Pero seré honesto, todavía no he hecho las investigaciones pertinentes exhaustivas así que inundar el texto con un output para acabar poniendo que todavía no lo sé no va a aportar nada hoy. Simplemente decirte que las flags usadas normalmente (hasta donde mi conocimiento llega) son estas: `readelf -l <binario>, readelf -S <binario>, readelf -h <binario>`.

Sí que pondré el output del `-h`, ya que podemos considerarlo un `file` pero más exhaustivo:

```bash
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
  Dirección del punto de entrada:    0x1070
  Inicio de encabezados de programa: 64  (bytes en el fichero)
  Inicio de encabezados de sección:  14112 (bytes en el fichero)
  Opciones:                          0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         14
  Size of section headers:           64 (bytes)
  Number of section headers:         31
  Section header string table index: 30
```

Mencionar concretamente lo mencionado ayer: los bytes fijos del formato ELF son estos: `7f 45 4c 46`, los siguientes nos dan información del mismo y el entry point cargado en 0x1070 de la memoria virtual.


## Análisis estático con objdump

Todavía no vamos a entrar en profundidad con Ghidra al ser ejecutables manejables en cuanto a tamaño. Aunque usar objdump no quita que puedas usar ghidra, es un preámbulo totalmente correcto. 

El comando a usar: `objdump -d -M intel <nombre_binario>`

Su output: 

```bash

test:     formato del fichero elf64-x86-64

Desensamblado de la sección .init:

0000000000001000 <_init>:
    1000:	48 83 ec 08          	sub    rsp,0x8
    1004:	48 8b 05 c5 2f 00 00 	mov    rax,QWORD PTR [rip+0x2fc5]        # 3fd0 <__gmon_start__@Base>
    100b:	48 85 c0             	test   rax,rax
    100e:	74 02                	je     1012 <_init+0x12>
    1010:	ff d0                	call   rax
    1012:	48 83 c4 08          	add    rsp,0x8
    1016:	c3                   	ret

Desensamblado de la sección .plt:

0000000000001020 <puts@plt-0x10>:
    1020:	ff 35 ca 2f 00 00    	push   QWORD PTR [rip+0x2fca]        # 3ff0 <_GLOBAL_OFFSET_TABLE_+0x8>
    1026:	ff 25 cc 2f 00 00    	jmp    QWORD PTR [rip+0x2fcc]        # 3ff8 <_GLOBAL_OFFSET_TABLE_+0x10>
    102c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]

0000000000001030 <puts@plt>:
    1030:	ff 25 ca 2f 00 00    	jmp    QWORD PTR [rip+0x2fca]        # 4000 <puts@GLIBC_2.2.5>
    1036:	68 00 00 00 00       	push   0x0
    103b:	e9 e0 ff ff ff       	jmp    1020 <_init+0x20>

0000000000001040 <strlen@plt>:
    1040:	ff 25 c2 2f 00 00    	jmp    QWORD PTR [rip+0x2fc2]        # 4008 <strlen@GLIBC_2.2.5>
    1046:	68 01 00 00 00       	push   0x1
    104b:	e9 d0 ff ff ff       	jmp    1020 <_init+0x20>

0000000000001050 <exit@plt>:
    1050:	ff 25 ba 2f 00 00    	jmp    QWORD PTR [rip+0x2fba]        # 4010 <exit@GLIBC_2.2.5>
    1056:	68 02 00 00 00       	push   0x2
    105b:	e9 c0 ff ff ff       	jmp    1020 <_init+0x20>

Desensamblado de la sección .plt.got:

0000000000001060 <__cxa_finalize@plt>:
    1060:	ff 25 7a 2f 00 00    	jmp    QWORD PTR [rip+0x2f7a]        # 3fe0 <__cxa_finalize@GLIBC_2.2.5>
    1066:	66 90                	xchg   ax,ax

Desensamblado de la sección .text:

0000000000001070 <_start>:
    1070:	31 ed                	xor    ebp,ebp
    1072:	49 89 d1             	mov    r9,rdx
    1075:	5e                   	pop    rsi
    1076:	48 89 e2             	mov    rdx,rsp
    1079:	48 83 e4 f0          	and    rsp,0xfffffffffffffff0
    107d:	50                   	push   rax
    107e:	54                   	push   rsp
    107f:	45 31 c0             	xor    r8d,r8d
    1082:	31 c9                	xor    ecx,ecx
    1084:	48 8d 3d eb 00 00 00 	lea    rdi,[rip+0xeb]        # 1176 <main>
    108b:	ff 15 2f 2f 00 00    	call   QWORD PTR [rip+0x2f2f]        # 3fc0 <__libc_start_main@GLIBC_2.34>
    1091:	f4                   	hlt
    1092:	66 2e 0f 1f 84 00 00 	cs nop WORD PTR [rax+rax*1+0x0]
    1099:	00 00 00 
    109c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]

00000000000010a0 <deregister_tm_clones>:
    10a0:	48 8d 3d 81 2f 00 00 	lea    rdi,[rip+0x2f81]        # 4028 <__TMC_END__>
    10a7:	48 8d 05 7a 2f 00 00 	lea    rax,[rip+0x2f7a]        # 4028 <__TMC_END__>
    10ae:	48 39 f8             	cmp    rax,rdi
    10b1:	74 15                	je     10c8 <deregister_tm_clones+0x28>
    10b3:	48 8b 05 0e 2f 00 00 	mov    rax,QWORD PTR [rip+0x2f0e]        # 3fc8 <_ITM_deregisterTMCloneTable@Base>
    10ba:	48 85 c0             	test   rax,rax
    10bd:	74 09                	je     10c8 <deregister_tm_clones+0x28>
    10bf:	ff e0                	jmp    rax
    10c1:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
    10c8:	c3                   	ret
    10c9:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]

00000000000010d0 <register_tm_clones>:
    10d0:	48 8d 3d 51 2f 00 00 	lea    rdi,[rip+0x2f51]        # 4028 <__TMC_END__>
    10d7:	48 8d 35 4a 2f 00 00 	lea    rsi,[rip+0x2f4a]        # 4028 <__TMC_END__>
    10de:	48 29 fe             	sub    rsi,rdi
    10e1:	48 89 f0             	mov    rax,rsi
    10e4:	48 c1 ee 3f          	shr    rsi,0x3f
    10e8:	48 c1 f8 03          	sar    rax,0x3
    10ec:	48 01 c6             	add    rsi,rax
    10ef:	48 d1 fe             	sar    rsi,1
    10f2:	74 14                	je     1108 <register_tm_clones+0x38>
    10f4:	48 8b 05 dd 2e 00 00 	mov    rax,QWORD PTR [rip+0x2edd]        # 3fd8 <_ITM_registerTMCloneTable@Base>
    10fb:	48 85 c0             	test   rax,rax
    10fe:	74 08                	je     1108 <register_tm_clones+0x38>
    1100:	ff e0                	jmp    rax
    1102:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    1108:	c3                   	ret
    1109:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]

0000000000001110 <__do_global_dtors_aux>:
    1110:	f3 0f 1e fa          	endbr64
    1114:	80 3d 0d 2f 00 00 00 	cmp    BYTE PTR [rip+0x2f0d],0x0        # 4028 <__TMC_END__>
    111b:	75 2b                	jne    1148 <__do_global_dtors_aux+0x38>
    111d:	55                   	push   rbp
    111e:	48 83 3d ba 2e 00 00 	cmp    QWORD PTR [rip+0x2eba],0x0        # 3fe0 <__cxa_finalize@GLIBC_2.2.5>
    1125:	00 
    1126:	48 89 e5             	mov    rbp,rsp
    1129:	74 0c                	je     1137 <__do_global_dtors_aux+0x27>
    112b:	48 8b 3d ee 2e 00 00 	mov    rdi,QWORD PTR [rip+0x2eee]        # 4020 <__dso_handle>
    1132:	e8 29 ff ff ff       	call   1060 <__cxa_finalize@plt>
    1137:	e8 64 ff ff ff       	call   10a0 <deregister_tm_clones>
    113c:	c6 05 e5 2e 00 00 01 	mov    BYTE PTR [rip+0x2ee5],0x1        # 4028 <__TMC_END__>
    1143:	5d                   	pop    rbp
    1144:	c3                   	ret
    1145:	0f 1f 00             	nop    DWORD PTR [rax]
    1148:	c3                   	ret
    1149:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]

0000000000001150 <frame_dummy>:
    1150:	f3 0f 1e fa          	endbr64
    1154:	e9 77 ff ff ff       	jmp    10d0 <register_tm_clones>

0000000000001159 <error>:
    1159:	55                   	push   rbp
    115a:	48 89 e5             	mov    rbp,rsp
    115d:	48 8d 05 a0 0e 00 00 	lea    rax,[rip+0xea0]        # 2004 <_IO_stdin_used+0x4>
    1164:	48 89 c7             	mov    rdi,rax
    1167:	e8 c4 fe ff ff       	call   1030 <puts@plt>
    116c:	bf 01 00 00 00       	mov    edi,0x1
    1171:	e8 da fe ff ff       	call   1050 <exit@plt>

0000000000001176 <main>:
    1176:	55                   	push   rbp
    1177:	48 89 e5             	mov    rbp,rsp
    117a:	48 83 ec 20          	sub    rsp,0x20
    117e:	89 7d ec             	mov    DWORD PTR [rbp-0x14],edi
    1181:	48 89 75 e0          	mov    QWORD PTR [rbp-0x20],rsi
    1185:	83 7d ec 02          	cmp    DWORD PTR [rbp-0x14],0x2
    1189:	75 19                	jne    11a4 <main+0x2e>
    118b:	48 8b 45 e0          	mov    rax,QWORD PTR [rbp-0x20]
    118f:	48 83 c0 08          	add    rax,0x8
    1193:	48 8b 00             	mov    rax,QWORD PTR [rax]
    1196:	48 89 c7             	mov    rdi,rax
    1199:	e8 a2 fe ff ff       	call   1040 <strlen@plt>
    119e:	48 83 f8 04          	cmp    rax,0x4
    11a2:	74 05                	je     11a9 <main+0x33>
    11a4:	e8 b0 ff ff ff       	call   1159 <error>
    11a9:	c7 45 fc 00 00 00 00 	mov    DWORD PTR [rbp-0x4],0x0
    11b0:	c7 45 f8 00 00 00 00 	mov    DWORD PTR [rbp-0x8],0x0
    11b7:	eb 23                	jmp    11dc <main+0x66>
    11b9:	48 8b 45 e0          	mov    rax,QWORD PTR [rbp-0x20]
    11bd:	48 83 c0 08          	add    rax,0x8
    11c1:	48 8b 10             	mov    rdx,QWORD PTR [rax]
    11c4:	8b 45 f8             	mov    eax,DWORD PTR [rbp-0x8]
    11c7:	48 98                	cdqe
    11c9:	48 01 d0             	add    rax,rdx
    11cc:	0f b6 00             	movzx  eax,BYTE PTR [rax]
    11cf:	0f be c0             	movsx  eax,al
    11d2:	83 e8 30             	sub    eax,0x30
    11d5:	01 45 fc             	add    DWORD PTR [rbp-0x4],eax
    11d8:	83 45 f8 01          	add    DWORD PTR [rbp-0x8],0x1
    11dc:	83 7d f8 03          	cmp    DWORD PTR [rbp-0x8],0x3
    11e0:	7e d7                	jle    11b9 <main+0x43>
    11e2:	83 7d fc 14          	cmp    DWORD PTR [rbp-0x4],0x14
    11e6:	75 11                	jne    11f9 <main+0x83>
    11e8:	48 8d 05 30 0e 00 00 	lea    rax,[rip+0xe30]        # 201f <_IO_stdin_used+0x1f>
    11ef:	48 89 c7             	mov    rdi,rax
    11f2:	e8 39 fe ff ff       	call   1030 <puts@plt>
    11f7:	eb 14                	jmp    120d <main+0x97>
    11f9:	48 8d 05 2f 0e 00 00 	lea    rax,[rip+0xe2f]        # 202f <_IO_stdin_used+0x2f>
    1200:	48 89 c7             	mov    rdi,rax
    1203:	e8 28 fe ff ff       	call   1030 <puts@plt>
    1208:	e8 4c ff ff ff       	call   1159 <error>
    120d:	b8 00 00 00 00       	mov    eax,0x0
    1212:	c9                   	leave
    1213:	c3                   	ret

Desensamblado de la sección .fini:

0000000000001214 <_fini>:
    1214:	48 83 ec 08          	sub    rsp,0x8
    1218:	48 83 c4 08          	add    rsp,0x8
    121c:	c3                   	ret
```

Como podemos ver a la izquierda del todo, están las direcciones de memoria virtual. Luego los opcodes (los bytes en crudo de las instrucciones a ejecutar) seguido de los bytes sueltos. Esto es mucho más fácil de calcular en Ghidra. Aun así, son los bytes que se suman a la dirección de memoria de la izquierda como podemos apreciar (por suma me refiero a la cantidad de bytes que hay justo a la derecha de la dirección de memoria, que a su vez nos da la nueva dirección para saber dónde empieza la siguiente instrucción). 

*Nota: un opcode es el byte o los bytes que identifica qué operación es (mov, sub, test, jmp...). Para referirnos a todos los bytes, vamos a llamarlos bytes de la instrucción.*

*Nota aparte: No tiene que ver con las word, dword o qword que describimos ayer. Eso describe el tamaño del dato que una instrucción en concreto maneja y lo de la nota de arriba describe cuánto ocupa la instrucción en memoria. Son dos cuentas de bytes distintas que se pueden confundir fácilmente.

Un ejemplo simple: 

```bash
    1000:	48 83 ec 08          	sub    rsp,0x8
    1004:	48 8b 05 c5 2f 00 00 	mov    rax,QWORD PTR [rip+0x2fc5]        # 3fd0 <__gmon_start__@Base>
```

Con la siguiente línea:

```bash
100b:	48 85 c0             	test   rax,rax
```

El formato hexadecimal tiene los 9 números del formato decimal, para luego ser el 10=a, 11=b, 12=c, 13=d, 14=e, 15=f.

Por eso vemos 100b. 

Volviendo al análisis, podemos ver las funciones que nos interesan a nosotros, que es el `main` y `error`. Todo lo otro que vemos también son funciones que están dentro del binario, compiladas (_start, _init, register_tm_clones... etc). Todo esto vive dentro de la sección .text . 

Recordemos brevemente cómo funcionaba la sintaxis de Intel para pasar ya al análisis dinámico, que es donde vamos a centrar el esfuerzo:

```text

  112b:	48 8b 3d ee 2e 00 00 	mov    rdi,QWORD PTR [rip+0x2eee] 

                                ^-^    ^-^ ^---------------------^
                                operación   destino, fuente

                                |
                                v

                                La operación a realizar desde la fuente hacia el destino


```


## Análisis dinámico con gdb

Como hicimos ayer mismo, vamos a pasar directamente a desensamblar el main y la función de error. Esto en la vida real no es así de fácil, pero como aprendizaje nos sirve para empezar. 

Aquí tenemos el output de las dos funciones desensambladas juntas:

```text
(gdb) disassemble main 
Dump of assembler code for function main:
   0x0000000000001176 <+0>: 	push   rbp
   0x0000000000001177 <+1>:	    mov    rbp,rsp
   0x000000000000117a <+4>:	    sub    rsp,0x20
   0x000000000000117e <+8>:	    mov    DWORD PTR [rbp-0x14],edi
   0x0000000000001181 <+11>:	mov    QWORD PTR [rbp-0x20],rsi
   0x0000000000001185 <+15>:	cmp    DWORD PTR [rbp-0x14],0x2
   0x0000000000001189 <+19>:	jne    0x11a4 <main+46>
   0x000000000000118b <+21>:	mov    rax,QWORD PTR [rbp-0x20]
   0x000000000000118f <+25>:	add    rax,0x8
   0x0000000000001193 <+29>:	mov    rax,QWORD PTR [rax]
   0x0000000000001196 <+32>:	mov    rdi,rax
   0x0000000000001199 <+35>:	call   0x1040 <strlen@plt>
   0x000000000000119e <+40>:	cmp    rax,0x4
   0x00000000000011a2 <+44>:	je     0x11a9 <main+51>
   0x00000000000011a4 <+46>:	call   0x1159 <error>
   0x00000000000011a9 <+51>:	mov    DWORD PTR [rbp-0x4],0x0
   0x00000000000011b0 <+58>:	mov    DWORD PTR [rbp-0x8],0x0
   0x00000000000011b7 <+65>:	jmp    0x11dc <main+102>
   0x00000000000011b9 <+67>:	mov    rax,QWORD PTR [rbp-0x20]
   0x00000000000011bd <+71>:	add    rax,0x8
   0x00000000000011c1 <+75>:	mov    rdx,QWORD PTR [rax]
   0x00000000000011c4 <+78>:	mov    eax,DWORD PTR [rbp-0x8]
   0x00000000000011c7 <+81>:	cdqe
   0x00000000000011c9 <+83>:	add    rax,rdx
   0x00000000000011cc <+86>:	movzx  eax,BYTE PTR [rax]
   0x00000000000011cf <+89>:	movsx  eax,al
   0x00000000000011d2 <+92>:	sub    eax,0x30
   0x00000000000011d5 <+95>:	add    DWORD PTR [rbp-0x4],eax
   0x00000000000011d8 <+98>:	add    DWORD PTR [rbp-0x8],0x1
   0x00000000000011dc <+102>:	cmp    DWORD PTR [rbp-0x8],0x3
   0x00000000000011e0 <+106>:	jle    0x11b9 <main+67>
   0x00000000000011e2 <+108>:	cmp    DWORD PTR [rbp-0x4],0x14
   0x00000000000011e6 <+112>:	jne    0x11f9 <main+131>
   0x00000000000011e8 <+114>:	lea    rax,[rip+0xe30]        # 0x201f
   0x00000000000011ef <+121>:	mov    rdi,rax
   0x00000000000011f2 <+124>:	call   0x1030 <puts@plt>
   0x00000000000011f7 <+129>:	jmp    0x120d <main+151>
   0x00000000000011f9 <+131>:	lea    rax,[rip+0xe2f]        # 0x202f
   0x0000000000001200 <+138>:	mov    rdi,rax
   0x0000000000001203 <+141>:	call   0x1030 <puts@plt>
   0x0000000000001208 <+146>:	call   0x1159 <error>
   0x000000000000120d <+151>:	mov    eax,0x0
   0x0000000000001212 <+156>:	leave
   0x0000000000001213 <+157>:	ret
End of assembler dump.
(gdb) disassemble error 
Dump of assembler code for function error:
   0x0000000000001159 <+0>: 	push   rbp
   0x000000000000115a <+1>:	    mov    rbp,rsp
   0x000000000000115d <+4>:	    lea    rax,[rip+0xea0]        # 0x2004
   0x0000000000001164 <+11>:	mov    rdi,rax
   0x0000000000001167 <+14>:	call   0x1030 <puts@plt>
   0x000000000000116c <+19>:	mov    edi,0x1
   0x0000000000001171 <+24>:	call   0x1050 <exit@plt>
End of assembler dump.
```

Espero hacerlo lo más ameno posible por si alguien estuviera empezando también con el reversing y no recuerde algunas cosas. Pero quizá por haber explicado algunas en el primer ejercicio de ayer me las pase, disculpas de antemano. 

Empezamos con el prólogo. ¿Qué es el prólogo? es donde se crea el marco de pila (stack frame) y la memoria para las variables que vaya a utilizar la función. 

```text
   0x0000000000001176 <+0>:	push   rbp
   0x0000000000001177 <+1>:	mov    rbp,rsp
   0x000000000000117a <+4>:	sub    rsp,0x20
```

En este ejercicio, lo identificamos en estas tres líneas. push rbp está creando el nuevo punto de anclaje para recordar dónde hay que regresar cuando la función termine, en este caso es para saber regresar a quien llamó al main. `rbp` es el frame pointer, siendo ebp en arquitecturas de 32 bits. 

En la segunda línea lo que estamos haciendo es crear el nuevo punto de anclaje. Con mov estamos copiando el valor de rsp a rbp. `rsp` es el stack pointer, esp en arquitecturas de 32 bits. A partir de aquí ya se pueden calcular los offsets de las variables locales en base al nuevo rbp.

Para terminar el prólogo, substraemos (o restamos) 0x20 a rsp, es decir, estamos creando memoria. Para calcular esto de manera rápida tenemos este comando, que nos devuelve 32 (son bytes que hemos reservado para las variables): 

```bash
echo "ibase=16; 20" | bc
32
```

Ahora empezamos con el código: 

```text
   0x000000000000117e <+8>:	    mov    DWORD PTR [rbp-0x14],edi
   0x0000000000001181 <+11>:	mov    QWORD PTR [rbp-0x20],rsi
   0x0000000000001185 <+15>:	cmp    DWORD PTR [rbp-0x14],0x2
   0x0000000000001189 <+19>:	jne    0x11a4 <main+46>
```

Aquí ya podemos identificar algo interesante. Primero de todo, copiamos edi a rbp-0x14. Es decir, ahora sabemos que rbp-0x14 pasa a valer argc (edi, por convención, es el registro que guarda el primer argumento de las funciones, siendo rdi el mismo registro pero para arquitecturas de 64 bits). Recordemos también las palabras (words): `DWORD PTR` es un double word pointer. Con esto entendemos que argc es un `int` de 4 bytes. 

Básicamente estamos preparando los argumentos aquí, ya que rsi es la parte "contraria" de rdi o edi: es para el segundo argumento de las funciones. En la segunda línea estamos copiando rsi a rbp-0x20, es decir, argv como tal. Sabemos que argv se suele declarar como char, así que el qword ptr en este caso guarda un puntero entero de 8 bytes. Puede que esto no lo haya explicado del todo correcto, ya que me sigue causando confusión a mí. 

Con los argumentos ya preparados en las dos primeras líneas, parece que entramos en una parte de control de errores del programa, por el cmp que podemos ver. Es decir, ahora vemos en la tercera línea el 'compare', que está comparando 2 a rbp-0x14 (recordemos que era argc). Así que básicamente lo que podemos ver de aquí es el típico `if (argc != 2)...` . La siguiente línea podemos deducir que va a saltar a la función de error casi con total certeza, siendo la instrucción `jne` jump if not equal. Si es igual seguimos con el flujo normal del programa, si no, saltemos al control de errores.

Como vemos que está en el main+46, al saltar a esa línea lo comprobamos claramente:

```text
0x00000000000011a4 <+46>:	call   0x1159 <error>
```

Y así ha sido. El 0x1159 lo comprobaremos luego, pero con total certeza será la primera línea del prólogo de la función de `error`, siendo un push rbp para crear ese nuevo punto de anclaje que mencionábamos anteriormente.

Para que sea más didáctico, a diferencia del primer ejercicio, creo que podría ir de 4 en 4 instrucciones, las ponemos y las analizamos. Siguiendo el flujo correcto del programa:

```text
   0x000000000000118b <+21>:	mov    rax,QWORD PTR [rbp-0x20]
   0x000000000000118f <+25>:	add    rax,0x8
   0x0000000000001193 <+29>:	mov    rax,QWORD PTR [rax]
   0x0000000000001196 <+32>:	mov    rdi,rax
```

En la siguiente línea de la instrucción jne, podemos ver ahora como estamos copiando el valor de rbp-0x20 a rax. `rax`, como dijimos, es un registro que se usa para guardar los resultados de las operaciones matemáticas o aritméticas, así que si no me equivoco, será común que lo veamos después de llamadas `call`. Vemos de nuevo el qword ptr. En este caso, sabemos que rax ahora guarda argv, un puntero de 8 bytes por el qword ptr.

Luego copiamos 8 a rax en la siguiente línea. Aprendiendo de los errores de ayer, sabemos que ahora rax apunta a argv[1], hemos movido el puntero 8 bytes. Esto lo que me hace plantearme de nuevo es si era argv[0] y no argv a secas, pero con repetición y teoría sacaremos esto rápido.

En la siguiente línea podemos ver como ahora copiamos rax a rax. Es decir, esas dos instrucciones van juntas y podemos ver claramente como hemos movido con la operación anterior el puntero para que apunte a argv[1], que cobrará más sentido en las siguientes instrucciones (ya que hemos visto que había strlen aquí también, por lo que podemos deducir con el strings que habíamos visto que compararemos argv[1] con la longitud de los dígitos del primer argumento (recordando también el "Usage..." que salía al principio del análisis)). 

Luego, copiamos rax a rdi con mov. `rdi` es un registro para los argumentos de las funciones, antes había dicho que era edi pero para 64 bits, es decir, que debería guardar el argc(el primer argumento) y no argv, creo que acabo de despistarme pero considero que esta duda está bien dejarla aquí. Por el momento, vamos a analizarlo pensando o sabiendo que rdi contiene argv[1] que lo acabamos de copiar de rax. 

Teniendo esto, las siguientes instrucciones son estas:

```text
   0x0000000000001199 <+35>:	call   0x1040 <strlen@plt>
   0x000000000000119e <+40>:	cmp    rax,0x4
   0x00000000000011a2 <+44>:	je     0x11a9 <main+51>
   0x00000000000011a4 <+46>:	call   0x1159 <error>
```

Bueno, aunque rdi tenga el valor de rax (argv[1]), deduzco que no borra su valor por lo que acabamos de ver. Primero, ejecutamos una llamada a una función que es strlen, que sabemos que sirve para medir la longitud del string. 

Esto son comandos en gdb que he ejecutado para ver qué me mostraba. Son los que habitualmente uso ahora mismo, por simple comprobación más que nada, hay bastantes cosas que no sé interpretar todavía o cómo leerlas (más allá del Assembly me refiero. Por ejemplo, para escanear direcciones de memoria. Si fuera un string el cual habita en .rodata, el análisis sería tan sencillo como poner x/s <dirección>, que significa "examine string de esta dirección"). 

Este era el output, que lo pongo por si alguien no domina los comandos, para que sepa cómo "guiarse":

```text
gdb) x/i 0x1040
   0x1040 <strlen@plt>:	jmp    QWORD PTR [rip+0x2fc2]        # 0x4008 <strlen@got.plt>
(gdb) x/s 0x1040
0x1040 <strlen@plt>:	"\377%\302/"
(gdb) x/10i 0x1040
   0x1040 <strlen@plt>:	jmp    QWORD PTR [rip+0x2fc2]        # 0x4008 <strlen@got.plt>
   0x1046 <strlen@plt+6>:	push   0x1
   0x104b <strlen@plt+11>:	jmp    0x1020
   0x1050 <exit@plt>:	jmp    QWORD PTR [rip+0x2fba]        # 0x4010 <exit@got.plt>
   0x1056 <exit@plt+6>:	push   0x2
   0x105b <exit@plt+11>:	jmp    0x1020
   0x1060 <__cxa_finalize@plt>:	jmp    QWORD PTR [rip+0x2f7a]        # 0x3fe0
   0x1066 <__cxa_finalize@plt+6>:	xchg   ax,ax
   0x1068:	Cannot access memory at address 0x1068
(gdb) x/8xb 0x1040
0x1040 <strlen@plt>:	0xff	0x25	0xc2	0x2f	0x00	0x00	0x68	0x01
(gdb) x/xw 0x1040
0x1040 <strlen@plt>:	0x2fc225ff
(gdb) x/xw 0x2fc225ff
0x2fc225ff:	Cannot access memory at address 0x2fc225ff
```

Siguiendo con el análisis de las 4 instrucciones anteriores, después de la llamada a strlen tenemos una comparación (`cmp   rax, 0x4`). Pensándolo bien, podríamos deducir de aquí que esto podría ser perfectamente un if (strlen(argv[1]) == 4) o algo por el estilo. Básicamente, lo que estamos haciendo es comparar 4 a rax, que es argv[1] (recordemos lo que he dicho antes, el "usage"). Si es exactamente 4, saltamos a main+51 (que este es el flujo "correcto" del programa). Si no, ejecuta una llamada a la función `error` que ya habíamos hecho un repaso antes en el `jne` del principio. No lo he dicho, pero `je` es la instrucción 'jump if equal'.

Las 4 siguientes instrucciones son estas:

```text
   0x00000000000011a9 <+51>:	mov    DWORD PTR [rbp-0x4],0x0
   0x00000000000011b0 <+58>:	mov    DWORD PTR [rbp-0x8],0x0
   0x00000000000011b7 <+65>:	jmp    0x11dc <main+102>
   0x00000000000011b9 <+67>:	mov    rax,QWORD PTR [rbp-0x20]
```

Vale, esta es la "clásica". Digo clásica porque en los binarios que he analizado es el patrón que vemos en las dos primeras instrucciones cuando creamos una variable y la inicializamos a cero. No quiero arriesgarme, pero ver las dos juntas puede hacerme pensar en un bucle for, pero es una suposición sin certeza ninguna (pensando algo como `int i, c; c = 0; for (i=0; i<10; i++;` por ejemplo). No me la voy a jugar y vamos a decir simplemente que se han inicializado dos variables. Esto es, estamos copiando el valor de 0 (con `mov`) a rbp-0x4 y rbp-0x8. Aquí es donde entra el concepto de desreferenciación que no quería nombrar a la ligera por mi ignorancia. Cuando vemos los corchetes, por norma general, lo que hacemos es desreferenciar, pero creo que depende del registro la desreferenciación se trata de manera distinta. En este caso, el 'destino' es un puntero que apunta a una dirección de memoria, y al desreferenciarlo lo que hacemos es obtener su valor o, en este caso, copiar un valor. Ahora esa dirección de memoria contiene el valor 0. 

Antes de analizar la última línea, debemos recordar que si vemos jmp (jump), es un salto incondicional, por lo tanto, debemos ir a esa dirección para seguir el hilo de ejecución, así que las 4 siguientes instrucciones van a ser las del main+102 en adelante, dejaremos la última que ha quedado antes para luego.

Estas son las siguientes 4:

```text
   0x00000000000011dc <+102>:	cmp    DWORD PTR [rbp-0x8],0x3
   0x00000000000011e0 <+106>:	jle    0x11b9 <main+67>
   0x00000000000011e2 <+108>:	cmp    DWORD PTR [rbp-0x4],0x14
   0x00000000000011e6 <+112>:	jne    0x11f9 <main+131>
```

Después de haber inicializado las dos variables anteriores del jmp, hay un cmp. Ahora estamos comparando si rbp-0x8 es igual a 3. Recordemos que esta era la segunda variable inicializada a cero. ¿Por qué compara a 3? no tengo certeza de esto. Quiero pensar que es porque empezamos a contar de cero y, por lo tanto, son 4 dígitos totales. 

En la siguiente instrucción tenemos `jle` -> jump if less or equal. Se hace la comparación anterior y, de ser menor o igual, volvemos a main+67. En el momento que sea igual o superior, continuaríamos con el flujo del programa. Así que lo que vamos a hacer es analizar las otras dos instrucciones e irnos a main+67. 

Las otras dos instrucciones hacen esto: primero comparamos el valor de 0x14 con la primera variable inicializada, el rbp-0x4. Si nos vamos al comando que había puesto antes de bash del ibase, veremos que nos da 20. Así que la primera variable la estamos comparando con 20, no sé muy bien por qué, pero esto es lo que nos dice el código máquina. Luego `jne` es jump if not equal. Si no es igual a 20, nos vamos al main+131. Estas dos instrucciones nos hacen pensar en un condicional. Si es igual a 20, saltamos. Es decir (if algo == 20) haz esto. 

Como ahora tenemos 2 frentes abiertos, vamos a analizar las instrucciones del main+67 que teníamos pendientes, pero esta vez las pondrée todas hasta el main+102 para que sea menos lioso para todos:

```text
   0x00000000000011b9 <+67>:	mov    rax,QWORD PTR [rbp-0x20]
   0x00000000000011bd <+71>:	add    rax,0x8
   0x00000000000011c1 <+75>:	mov    rdx,QWORD PTR [rax]
   0x00000000000011c4 <+78>:	mov    eax,DWORD PTR [rbp-0x8]
   0x00000000000011c7 <+81>:	cdqe
   0x00000000000011c9 <+83>:	add    rax,rdx
   0x00000000000011cc <+86>:	movzx  eax,BYTE PTR [rax]
   0x00000000000011cf <+89>:	movsx  eax,al
   0x00000000000011d2 <+92>:	sub    eax,0x30
   0x00000000000011d5 <+95>:	add    DWORD PTR [rbp-0x4],eax
   0x00000000000011d8 <+98>:	add    DWORD PTR [rbp-0x8],0x1
   0x00000000000011dc <+102>:	cmp    DWORD PTR [rbp-0x8],0x3
```

Aquí tenemos bastante chicha. Primero, estamos copiando el valor de rbp-0x20 a rax. Si recordamos al principio (y este registro mirando el desensamblado no veo que haya cambiado), debería contener argv. Diría que estas líneas no las habíamos analizado aún pese a que son muy parecidas a las instrucciones del principio, las primeras 15. Vemos que se repite el patrón, si rbp-0x20 es argv, cuando le añadimos 8 estamos moviendo un puntero que contiene 8 bytes a rax. Es decir, ahora rax apunta a argv[1].  

Luego copiamos el valor de lo que contiene rax (casi con total certeza, por la desreferenciación) a rdx. `rdx` es un registro que actualmente no recuerdo, haré la investigación cuando termine el análisis para que sea lícito el análisis. Básicamente, si no me equivoco, estamos copiando el primer argumento que le hemos pasado al binario a rdx. Y luego, copiamos el valor de lo que contiene la segunda variable que se había creado a eax. Misma lógica, copiamos el valor, no una dirección de memoria.

Ahora bien, estos 3 registros que acaban de aparecer es la primera vez que los veo. No sé qué hace cdqe, movzx ni movsx. Así que estas instrucciones, también para el lector si no las conoce, quedan pendientes de ir a investigarlas. 

La que está en medio, add, sabemos que añade el valor. En este caso, estamos añadiendo lo que vale rdx a rax, que contiene argv[1]. Esta parte sí que se nos va a complicar. Como acabo de decir lo que contiene rdx, creo que estamos simplemente guardando el argv[1] en un registro aparte por el bien del flujo del programa. Si movzx y movsx guardan la misma raíz, vamos a decir que en ese par de instrucciones se está copiando un byte de rax a eax (eax es lo mismo que rax, siendo eax el registro en arquitecturas de 32 bits). Podemos pensar que estamos avanzando en un array. Lo siguiente, copia el valor de al a eax, aunque son simples suposiciones. `al` lo había visto con anterioridad y pensaba que era para hacer comparaciones con test, creo que era un registro que guardaba 0 de valor. Pero esto son suposiciones.

Las siguientes instrucciones, hace que restemos 0x30 (que es 48 en decimal) a eax. Iba a poner que no tenía ni idea de esto, pero hemos restado tantas veces en el k&r que este comportamiento lo que hace es restar por 0 un caracter. Es decir, estamos convirtiendo caracteres a enteros. Cuando le restamos 0, obtenemos su valor en decimal real. Casi con certeza debe ser esto. 

Luego añadimos el valor de eax a rbp-0x4 que era la primera variable. No sé la finalidad de esto, la verdad. Luego hacemos lo mismo con la segunda variable que habíamos identificado, rbp-0x8, sumándole esta vez 1 byte, para después proceder a comparar esto último con 3. 

Siendo totalmente honesto aquí estoy un poco perdido, pero esto se repite hasta que, como habíamos visto antes, sea menor o igual que 3. Vamos a analizar las últimas instrucciones:

```text
   0x00000000000011e0 <+106>:	jle    0x11b9 <main+67>
   0x00000000000011e2 <+108>:	cmp    DWORD PTR [rbp-0x4],0x14
   0x00000000000011e6 <+112>:	jne    0x11f9 <main+131>
   0x00000000000011e8 <+114>:	lea    rax,[rip+0xe30]        # 0x201f
   0x00000000000011ef <+121>:	mov    rdi,rax
   0x00000000000011f2 <+124>:	call   0x1030 <puts@plt>
   0x00000000000011f7 <+129>:	jmp    0x120d <main+151>
   0x00000000000011f9 <+131>:	lea    rax,[rip+0xe2f]        # 0x202f
   0x0000000000001200 <+138>:	mov    rdi,rax
   0x0000000000001203 <+141>:	call   0x1030 <puts@plt>
   0x0000000000001208 <+146>:	call   0x1159 <error>
   0x000000000000120d <+151>:	mov    eax,0x0
   0x0000000000001212 <+156>:	leave
   0x0000000000001213 <+157>:	ret
```

El `jle` sabemos que iba después del main+102, así que nos lo saltamos ahora. Luego, cuando este bucle que hemos visto termina, hacemos una comparación con el compare. Estamos viendo si el resultado de lo anterior es igual a 20.

Vemos ahora otro jne. Si no es igual, nos vamos a main+131, que es el que vamos a analizar ahora. Vemos que contiene la instrucción lea, que es load effective address. Aquí no desreferenciamos en el sentido de obtener el valor, solo se cargan direcciones de memoria pese a ver los corchetes. Ahora bien, que contiene esa dirección relativa? vemos el famoso rip (el instruction pointer), que contiene la dirección de memoria de la siguiente instrucción a ejecutar. Veamos gdb que nos muestra (lo que vemos después del hastag es el cálculo ya hecho por gdb):


```text
(gdb) x/i 0x202f
   0x202f:	rex.XB outs dx,DWORD PTR ds:[rsi]
(gdb) x/8xb 0x202f
0x202f:	0x43	0x6f	0x64	0x69	0x67	0x6f	0x20	0x69
(gdb) x/s 0x202f
0x202f:	"Codigo incorrecto"
```

Creo que la primera instrucción es un error porque, al ser un string, gdb intenta interpretar los datos y aún así te lo pone, pero diría que está mal si lo examinamos como instrucción. Cuando vemos los bytes sueltos, si le has echado un ojo a la tabla ASCII, puedes identificar que son caracteres, que es lo que nos muestra el último comando de gdb. Es decir, era un control de errores. Si la primera variable (rbp-0x4) no era igual a 20, saltamos a un posible else que nos muestra el código incorrecto. 


Vamos a analizar también qué dice la cuarta instrucción de arriba, que si es igual no saltamos a la otra dirección. Es un lea igual que anterior y cargamos esta dirección en rax. Por pura lógica y lo que habíamos visto con el comando strings, aquí debe haber "codigo correcto", y lo vamos a comprobar ahora mismo directamente ya que nos permite esto gdb:

```text
(gdb) x/s 0x201f
0x201f:	"Codigo correcto"
```

Efectivamente. Las siguientes instrucciones copian rax a rdi (el codigo correcto), y llama a la función puts que era para printf. Es decir, printf("Codigo correcto") es lo que se está imprimiendo en esa secuencia. Después saltamos a main+151, copia 0 a eax y entramos en el epílogo, que prepara la función para poder salir. Este mov eax,0 del final debe ser el típico return 0 del final de la función main.

Vale, creo que ha ido bastante bien hoy la verdad, vamos a analizar la función de error y ponemos las conclusiones:

```text
(gdb) disassemble error 
Dump of assembler code for function error:
   0x0000000000001159 <+0>: 	push   rbp
   0x000000000000115a <+1>:	    mov    rbp,rsp
   0x000000000000115d <+4>:	    lea    rax,[rip+0xea0]        # 0x2004
   0x0000000000001164 <+11>:	mov    rdi,rax
   0x0000000000001167 <+14>:	call   0x1030 <puts@plt>
   0x000000000000116c <+19>:	mov    edi,0x1
   0x0000000000001171 <+24>:	call   0x1050 <exit@plt>
End of assembler dump.
```

El prólogo en esta función parecen ser las dos primeras líneas simplemente. Como hemos dicho, creamos un punto de anclaje para poder volver a la función que nos llamó (main en este caso), y creamos el nuevo punto de anclaje. rbp -> frame pointer, rsp -> stack pointer. Ya se ha creado el marco de pila. 

Basándome en lo que hemos visto con la función main, es más que probable que en la tercera instrucción (la de lea), es donde se esté cargando el string que habíamos visto de "usage:..." con strings. Hacemos una comprobación rápida y seguimos:

```text
(gdb) x/s 0x2004
0x2004:	"Usage: <binario> 4 digitos"
```

¡Correcto! dudo que sea un patrón que se repita, pero lea en todos los casos que hemos visto tanto ayer como hoy ha cargado direcciones de memoria que contienen strings.

En la siguiente instrucción podemos ver que estamos copiando rax a rdi. Puedo leerlo pero no sé aquí qué contiene rax. Luego llamamos a la función de puts (printf) e imprime lo del Usage. Se había preparado el terreno con las instrucciones anteriores. Luego, copiamos 1 a edi (mov edi,0x1) y llamamos a la función call. Básicamente, es como poner exit(1) o exit(EXIT_FAILURE). 

## Conclusión

Conclusiones: Personalmente, como he dicho al principio y ayer, poder llegar a abstraerlo todo bien y entender cómo era el código fuente sin tenerlo es complejo. Creo que hoy ha ido bien e incluso mejor que ayer que sí que lo teníamos delante, pero debo decir que también era un ejercicio muy parecido el que nos había sugerido el LLM. Supongo que esta habilidad para poder entender cómo funciona un binario sin tener el código fuente o simplemente leyendo Assembly se entrena con el tiempo. Luego, también debo añadir que a veces me cuesta seguir lo que podría contener un registro si el binario fuera más largo, y no descarto que me haya pasado en este.  


Ahora, teniendo el código delante, ¡considero que no ha ido tan mal! no hemos sabido identificar el || del principio como OR (aunque hayamos leído las instrucciones correctamente), pero sí el bucle for! Falta mucho camino todavía, pero poco a poco. 


