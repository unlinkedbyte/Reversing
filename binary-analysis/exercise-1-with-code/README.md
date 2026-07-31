### Primeros pasos en reversing: destripando un binario del que sabemos el código fuente

*Nota: este primer writeup tiene varios errores conceptuales, corregidos en el segundo ejercicio (el análisis a ciegas). Los dejo tal cual para comparar un análisis con otro y ver la mejora real, no solo el resultado final.*

### Estructura

Bueno, para empezar, debemos estructurar cómo, en teoría, debemos enfocar el pre-análisis del binario. En este caso recordábamos el código fuente porque lo había escrito hacía unos días para coger algo de soltura. Quiero añadir una cosa: como veréis en el código, hay una redundancia en el primer control de errores del main, quería poder ver cómo se traducía esto en assembly. Esta redundancia que menciono se traduce en dos `cmp + jg` (que era previsible si lo pensamos), mientras que el if (argc != 2) se traduce a una única comparación + salto: cmp destino, 0x2. Es una huella visible que creo que estaba bien mencionar, el malware ofuscado a veces añade "estas tonterías" u operaciones totalmente redundantes para despistar al ingeniero inverso. 

Dicho esto, esta es la estructura que estoy siguiendo, sus respectivos outputs y qué entendemos (por el momento) de cada uno de ellos: 

1. **file <nombre_binario>**:

Esto nos sirve para identificar el tipo del binario. Este es su output:

```bash
clave: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=e577855a48b53ec0938cd7df81ea14556f864d7f, for GNU/Linux 3.2.0, not stripped
```

**Qué podemos averiguar de aquí a simple vista?**

Es un formato ELF de 64 bits, lo que nos indica que es un binario compilado en linux. Quiero matizar que no debes preocuparte por no recordar todas las nomenclaturas al principio. Poco a poco, con identificar para qué sirva cada una al principio vas bien. LSB nos indica que es little endian, y pie executable nos indica que las direcciones de memoria van a moverse (básicamente, el ASLR se aplica gracias a esto). La versión 1 (SYSV, sys five), nos indicaba la ABI, que si no recuerdo mal era el intérprete que se usa para los registros de la arquitectura de nuestro procesador. Como vemos, esta enlazado dinámicamente (esto es por tema de dependencias. Así, si se actualiza alguna librería, no rompe ningún binario de nuestro sistema). Luego podemos ver el intérprete, seguido de una clave, la versión del sistema operativo y que no esta stripped (un binario stripped ha ocultado, entre otras cosas, sus símbolos. Hace mas díficil la lectura y análisis del binario).

2. **strings <nombre_binario>**: 

Aquí queremos ver el texto legible que hay en el binario, nos suele dar mucha informacion del mismo. Podemos ver las secciones, las funciones, texto plano... Este es el output:

```bash
/lib64/ld-linux-x86-64.so.2
puts
exit
__libc_start_main
__cxa_finalize
strcmp
libc.so.6
GLIBC_2.2.5
GLIBC_2.34
_ITM_deregisterTMCloneTable
__gmon_start__
_ITM_registerTMCloneTable
PTE1
u+UH
Usage: <binary name> 'reversing'.
reversing
Acceso concedido
Acceso denegado
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
clave_por_argumento.c
__FRAME_END__
_DYNAMIC
__GNU_EH_FRAME_HDR
_GLOBAL_OFFSET_TABLE_
__libc_start_main@GLIBC_2.34
_ITM_deregisterTMCloneTable
puts@GLIBC_2.2.5
_edata
_fini
__data_start
strcmp@GLIBC_2.2.5
__gmon_start__
__dso_handle
_IO_stdin_used
_end
error_message
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

Hay muchísima información que todavía no controlo. Para empezar, nosotros nos fijaremos en .text (donde vive nuestro código), .rodata (que es donde estan todos los strings), .bss (que son las variables globales y estáticas no inicializadas), .data (que son las variables globales y estáticas sí inicializadas). Cabe destacar una cosa, un pequeño chivatado en esta linea: `GCC: (Debian 14.2.0-19) 14.2.0` que nos indica con qué versión se compiló (esto nos puede servir como pista porque nos puede indicar distintos patrones ligeramente distintos de código según cómo se haya compilado). Esto se incrusta en .comment .

Dicho esto, dejando de lado las secciones, podemos ver ciertas funciones como puts, exit y texto hardcoded, y cerca del main error_message que es una funcion hecha por nosotros. Puts no es que sea printf, pero cuando lo veamos, sabremos que el código fuente probablemente incluya algun printf si no varios, con strings fijos sin variables. Esto es debido a una automatización automática del compilador, ya que no necesita parsear formato.
Los strings, con el "Usage:..." y el acceso concedido y denegado, ya nos da una idea de la función del binario.

3. **readelf -h <nombre_binario>, readelf -S <nombre_binario> y readelf -l <nombre_binario>:**

readelf es la herramienta que nos enseña la estructura y metadatos del propio archivo ELF. Podemos ver que secciones tiene este binario, en qué dirección de memoria empieza cada una, qué segmentos hay que el kernel debe cargar al ejecutar, qué símbolos exporta o importa...

Los outputs serán un poco largos, así que empezaremos de menos a más. Antes de nada, un pequeño resumen de cada uno:

3.1. `readelf -h` --> el header ELF (arquitectura, entry point, tipo...)
3.2. `readelf -S` --> secciones con sus direcciones (.text, .rodata, .bss)
3.3. `readelf -l` --> los program headers, segmentos que el kernel mapea en memoria al hacer execve(). Las secciones y segmentos son cosas distintas.

Aunque quiero añadir antes de nada que, después de una pequeña investigación, en teoria los ingenieros inversos senior lo suelen usar por motivos concretos. Nos da una huella rapida antes de abrir herramientas pesadas como Ghidra para el posterior análisis estático. Es todo lo que identificamos con el comando `file` pero con mucho más detalle. También para hacer comprobaciones de seguridad (si hay stack canary, si .dynamic tiene GNU_RELRO o GNU_STACK marcado como no ejecutable. Esto es lo que hacen herramientas como checksec por debajo, automatizan lecturas de readelf y objdump). La parte más relevante que mencionaría es para cuando trabajemos con binarios stripped donde no sepamos dónde está el main: con readelf -l podemos localizar los segmentos que el kernel realmente carga (los LOAD que veremos en el output pertinente), relevante cuando queramos entender el mapa de memoria antes de un análisis dinámico (complejo a simple vista). También he leido que se usa en análisis de malware. A veces podrás encontrarte binarios con cabeceras ELF corruptas en campos no esenciales, y el cargador del kernel es tolerante y las ejecuta igual, pero herramientas automatizadas de análisis que parsean el header hace que den resultados falsos o peten. Por eso debemos tener en cuenta también cual es el header de un binario por si alguna herramienta automática pudiera fallar. Es este: `7f 45 4c 46` son los números mágicos fijos. 


**Output de 'readelf -h <nombre_binario>:**

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
  Inicio de encabezados de sección:  14120 (bytes en el fichero)
  Opciones:                          0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         14
  Size of section headers:           64 (bytes)
  Number of section headers:         31
  Section header string table index: 30

```

Como comentaba con anterioridad, podemos fijarnos en el encabezado, en los magic bytes. Qué son el 02, 01, 01...? Después de una búsqueda rápida lo encontramos: El siguiente byte (`02`) es la clase (2=64 bits), el siguiente (`01`) es el orden de bytes (1=little endian), y el siguiente (01) es la versión del formato ELF. Si el byte de clase fuera 01 en vez de 02, sería ELFCLASS32 (32 bits). Y si el siguiente byte fuera 02 en vez de 01, sería big endian (ELFDATA2MSB). 

Considero este output bueno, "sencillo" de entender y útil en cuanto a información relevante para una primera pasada. Vemos el entry point, vemos la ABI, vemos el sistema operativo... Aunque las últimas partes no las comprendo del todo todavía. 

**Output de 'readelf -l <nombre_binario>:**

```bash
El tipo del fichero elf es DYN (Position-Independent Executable file)
Entry point 0x1070
There are 14 program headers, starting at offset 64

Encabezados de Programa:
  Tipo           Desplazamiento     DirVirtual         DirFísica
                 TamFichero         TamMemoria          Opts   Alineación
  PHDR           0x0000000000000040 0x0000000000000040 0x0000000000000040
                 0x0000000000000310 0x0000000000000310  R      0x8
  INTERP         0x0000000000000394 0x0000000000000394 0x0000000000000394
                 0x000000000000001c 0x000000000000001c  R      0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
  LOAD           0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000698 0x0000000000000698  R      0x1000
  LOAD           0x0000000000001000 0x0000000000001000 0x0000000000001000
                 0x00000000000001f1 0x00000000000001f1  R E    0x1000
  LOAD           0x0000000000002000 0x0000000000002000 0x0000000000002000
                 0x0000000000000178 0x0000000000000178  R      0x1000
  LOAD           0x0000000000002dd0 0x0000000000003dd0 0x0000000000003dd0
                 0x0000000000000258 0x0000000000000260  RW     0x1000
  DYNAMIC        0x0000000000002de0 0x0000000000003de0 0x0000000000003de0
                 0x00000000000001e0 0x00000000000001e0  RW     0x8
  NOTE           0x0000000000000350 0x0000000000000350 0x0000000000000350
                 0x0000000000000020 0x0000000000000020  R      0x8
  NOTE           0x0000000000000370 0x0000000000000370 0x0000000000000370
                 0x0000000000000024 0x0000000000000024  R      0x4
  NOTE           0x0000000000002158 0x0000000000002158 0x0000000000002158
                 0x0000000000000020 0x0000000000000020  R      0x4
  GNU_PROPERTY   0x0000000000000350 0x0000000000000350 0x0000000000000350
                 0x0000000000000020 0x0000000000000020  R      0x8
  GNU_EH_FRAME   0x0000000000002058 0x0000000000002058 0x0000000000002058
                 0x0000000000000034 0x0000000000000034  R      0x4
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10
  GNU_RELRO      0x0000000000002dd0 0x0000000000003dd0 0x0000000000003dd0
                 0x0000000000000230 0x0000000000000230  R      0x1

 Asignación de Sección a Segmento:
  Segmento Secciones...
   00     
   01     .interp 
   02     .note.gnu.property .note.gnu.build-id .interp .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r .rela.dyn .rela.plt 
   03     .init .plt .plt.got .text .fini 
   04     .rodata .eh_frame_hdr .eh_frame .note.ABI-tag 
   05     .init_array .fini_array .dynamic .got .got.plt .data .bss 
   06     .dynamic 
   07     .note.gnu.property 
   08     .note.gnu.build-id 
   09     .note.ABI-tag 
   10     .note.gnu.property 
   11     .eh_frame_hdr 
   12     
   13     .init_array .fini_array .dynamic .got 
```
Pese a que aquí vemos lo que comentaba antes sobre GNU_RELRO y GNU_STACK, todavía me falta bastante información sobre este campo (todo este output). Lo voy a dejar como pendiente de investigación para que no quede un post tremendamente largo, pero si como evidencia para futuras investigaciones o anclaje para una futura búsqueda.

**Output de 'readelf -S <nombre_binario>:**

```bash
There are 31 section headers, starting at offset 0x3728:

Encabezados de Sección:
  [Nr] Nombre            Tipo             Dirección         Despl
       Tamaño            TamEnt           Opts   Enl   Info  Alin
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .note.gnu.pr[...] NOTE             0000000000000350  00000350
       0000000000000020  0000000000000000   A       0     0     8
  [ 2] .note.gnu.bu[...] NOTE             0000000000000370  00000370
       0000000000000024  0000000000000000   A       0     0     4
  [ 3] .interp           PROGBITS         0000000000000394  00000394
       000000000000001c  0000000000000000   A       0     0     1
  [ 4] .gnu.hash         GNU_HASH         00000000000003b0  000003b0
       0000000000000024  0000000000000000   A       5     0     8
  [ 5] .dynsym           DYNSYM           00000000000003d8  000003d8
       00000000000000d8  0000000000000018   A       6     1     8
  [ 6] .dynstr           STRTAB           00000000000004b0  000004b0
       0000000000000099  0000000000000000   A       0     0     1
  [ 7] .gnu.version      VERSYM           000000000000054a  0000054a
       0000000000000012  0000000000000002   A       5     0     2
  [ 8] .gnu.version_r    VERNEED          0000000000000560  00000560
       0000000000000030  0000000000000000   A       6     1     8
  [ 9] .rela.dyn         RELA             0000000000000590  00000590
       00000000000000c0  0000000000000018   A       5     0     8
  [10] .rela.plt         RELA             0000000000000650  00000650
       0000000000000048  0000000000000018  AI       5    24     8
  [11] .init             PROGBITS         0000000000001000  00001000
       0000000000000017  0000000000000000  AX       0     0     4
  [12] .plt              PROGBITS         0000000000001020  00001020
       0000000000000040  0000000000000010  AX       0     0     16
  [13] .plt.got          PROGBITS         0000000000001060  00001060
       0000000000000008  0000000000000008  AX       0     0     8
  [14] .text             PROGBITS         0000000000001070  00001070
       0000000000000178  0000000000000000  AX       0     0     16
  [15] .fini             PROGBITS         00000000000011e8  000011e8
       0000000000000009  0000000000000000  AX       0     0     4
  [16] .rodata           PROGBITS         0000000000002000  00002000
       0000000000000055  0000000000000000   A       0     0     8
  [17] .eh_frame_hdr     PROGBITS         0000000000002058  00002058
       0000000000000034  0000000000000000   A       0     0     4
  [18] .eh_frame         PROGBITS         0000000000002090  00002090
       00000000000000c8  0000000000000000   A       0     0     8
  [19] .note.ABI-tag     NOTE             0000000000002158  00002158
       0000000000000020  0000000000000000   A       0     0     4
  [20] .init_array       INIT_ARRAY       0000000000003dd0  00002dd0
       0000000000000008  0000000000000008  WA       0     0     8
  [21] .fini_array       FINI_ARRAY       0000000000003dd8  00002dd8
       0000000000000008  0000000000000008  WA       0     0     8
  [22] .dynamic          DYNAMIC          0000000000003de0  00002de0
       00000000000001e0  0000000000000010  WA       6     0     8
  [23] .got              PROGBITS         0000000000003fc0  00002fc0
       0000000000000028  0000000000000008  WA       0     0     8
  [24] .got.plt          PROGBITS         0000000000003fe8  00002fe8
       0000000000000030  0000000000000008  WA       0     0     8
  [25] .data             PROGBITS         0000000000004018  00003018
       0000000000000010  0000000000000000  WA       0     0     8
  [26] .bss              NOBITS           0000000000004028  00003028
       0000000000000008  0000000000000000  WA       0     0     1
  [27] .comment          PROGBITS         0000000000000000  00003028
       000000000000001f  0000000000000001  MS       0     0     1
  [28] .symtab           SYMTAB           0000000000000000  00003048
       00000000000003a8  0000000000000018          29    18     8
  [29] .strtab           STRTAB           0000000000000000  000033f0
       000000000000021b  0000000000000000           0     0     1
  [30] .shstrtab         STRTAB           0000000000000000  0000360b
       000000000000011a  0000000000000000           0     0     1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), l (large), p (processor specific)

```

Mismo comentario que el de antes.

Ahora pasaremos al análisis estático. Vamos a dejar Ghidra a un lado hoy ya que el código fuente es corto. Aunque fueramos a abrir ghidra, tener un primer escaneo con objdump es buena práctica.

### Análisis estático

4. **objdump -d -M intel <nombre_binario>**

Aquí hay una distinción que considero importante señalar. objdump es, en espíritu, Ghidra sin interfaz gráfica, pero hay una diferencia importante: objdump nos da el desensamblado plano instrucción por instrucción, sin interpretar nada más allá de eso. Ghidra e IDA además hacen análisis y reconstrucción (agrupan instrucciones en bloques básicos, reconstruyen el grafo del control de flujo, infireren tipos de variables y generan pseudocódigo en lenguaje parecido a C (el famoso decompile panel). Si queremos entrenar el ojo (para ver patrones crudos en ensamblador), empezar por objdump es perfecto porque no nos da nada masticado. 

**Output del comando y análisis estático:**

```bash
clave:     formato del fichero elf64-x86-64
^    ^
------
(este es el nombre
de nuestro binario)


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

0000000000001040 <strcmp@plt>:
    1040:	ff 25 c2 2f 00 00    	jmp    QWORD PTR [rip+0x2fc2]        # 4008 <strcmp@GLIBC_2.2.5>
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

0000000000001159 <error_message>:
    1159:	55                   	push   rbp
    115a:	48 89 e5             	mov    rbp,rsp
    115d:	48 8d 05 a4 0e 00 00 	lea    rax,[rip+0xea4]        # 2008 <_IO_stdin_used+0x8>
    1164:	48 89 c7             	mov    rdi,rax
    1167:	e8 c4 fe ff ff       	call   1030 <puts@plt>
    116c:	bf 01 00 00 00       	mov    edi,0x1
    1171:	e8 da fe ff ff       	call   1050 <exit@plt>

0000000000001176 <main>:
    1176:	55                   	push   rbp
    1177:	48 89 e5             	mov    rbp,rsp
    117a:	48 83 ec 10          	sub    rsp,0x10
    117e:	89 7d fc             	mov    DWORD PTR [rbp-0x4],edi
    1181:	48 89 75 f0          	mov    QWORD PTR [rbp-0x10],rsi
    1185:	83 7d fc 02          	cmp    DWORD PTR [rbp-0x4],0x2
    1189:	7f 06                	jg     1191 <main+0x1b>
    118b:	83 7d fc 01          	cmp    DWORD PTR [rbp-0x4],0x1
    118f:	7f 05                	jg     1196 <main+0x20>
    1191:	e8 c3 ff ff ff       	call   1159 <error_message>
    1196:	48 8b 45 f0          	mov    rax,QWORD PTR [rbp-0x10]
    119a:	48 83 c0 08          	add    rax,0x8
    119e:	48 8b 00             	mov    rax,QWORD PTR [rax]
    11a1:	48 8d 15 82 0e 00 00 	lea    rdx,[rip+0xe82]        # 202a <_IO_stdin_used+0x2a>
    11a8:	48 89 d6             	mov    rsi,rdx
    11ab:	48 89 c7             	mov    rdi,rax
    11ae:	e8 8d fe ff ff       	call   1040 <strcmp@plt>
    11b3:	85 c0                	test   eax,eax
    11b5:	75 16                	jne    11cd <main+0x57>
    11b7:	48 8d 05 76 0e 00 00 	lea    rax,[rip+0xe76]        # 2034 <_IO_stdin_used+0x34>
    11be:	48 89 c7             	mov    rdi,rax
    11c1:	e8 6a fe ff ff       	call   1030 <puts@plt>
    11c6:	b8 00 00 00 00       	mov    eax,0x0
    11cb:	eb 19                	jmp    11e6 <main+0x70>
    11cd:	48 8d 05 71 0e 00 00 	lea    rax,[rip+0xe71]        # 2045 <_IO_stdin_used+0x45>
    11d4:	48 89 c7             	mov    rdi,rax
    11d7:	e8 54 fe ff ff       	call   1030 <puts@plt>
    11dc:	e8 78 ff ff ff       	call   1159 <error_message>
    11e1:	b8 00 00 00 00       	mov    eax,0x0
    11e6:	c9                   	leave
    11e7:	c3                   	ret

Desensamblado de la sección .fini:

00000000000011e8 <_fini>:
    11e8:	48 83 ec 08          	sub    rsp,0x8
    11ec:	48 83 c4 08          	add    rsp,0x8
    11f0:	c3                   	ret
```

Considero, para empezar, usar esto mucho mejor que ghidra por el tema de la legibilidad aunque no sepas calcular los offsets o desplazamientos entre opcodes (que son las instrucciones traducidas a bytes).Como es un binario que no está stripped, y además en este conocemos el código fuente, ya sabemos las dos partes que, de primeras, nos interesan; y en las que nos vamos a centrar hoy, dado que todas las demás tienen que ver con el loader y con cómo se cargan en memoria. Ya con esto podríamos pasar a analizar Assembly, el desensamblado, pero vamos a abrir gdb para poder inspeccionar algunas instrucciones que no nos queden claras.


### Análisis dinámico

5. **gdb -q <nombre_binario>**

Una vez tenemos el primer mapeo con el análisis estático, procedemos al análisis dinámico (que, además, nos sirve como práctica para los comandos de gdb, por si a alguien se le pudiera hacer un mundo al principio). En el libro de jon erikson, donde empezamos a practicar el día que subí el primer post, él usaba la flag -g al compilar el binario y luego usaba el comando list para que fuera todo mas ilustrativo, pero nos dejaremos eso en este y los posteriores análisis.

Una vez abierto gdb, para esta sesión sabiendo que tenemos la función del main y de error_message en texto claro y sabemos que existen, directamente vamos a desensamblarlas y a poner el output para el posterior análisis con nuestras propias palabras. Añadiré una nota creo que a modo de conclusión al final de todo, que toca un tema importante y es un matiz complejo sobre abstracción. 

Aquí tenemos el desemsamblado de ambas funciones:

```
gdb -q ./clave
Reading symbols from ./clave...
(No debugging symbols found in ./clave)
(gdb) disassemble main 
Dump of assembler code for function main:
   0x0000000000001176 <+0>:	push   rbp
   0x0000000000001177 <+1>:	mov    rbp,rsp
   0x000000000000117a <+4>:	sub    rsp,0x10
   0x000000000000117e <+8>:	mov    DWORD PTR [rbp-0x4],edi
   0x0000000000001181 <+11>:	mov    QWORD PTR [rbp-0x10],rsi
   0x0000000000001185 <+15>:	cmp    DWORD PTR [rbp-0x4],0x2
   0x0000000000001189 <+19>:	jg     0x1191 <main+27>
   0x000000000000118b <+21>:	cmp    DWORD PTR [rbp-0x4],0x1
   0x000000000000118f <+25>:	jg     0x1196 <main+32>
   0x0000000000001191 <+27>:	call   0x1159 <error_message>
   0x0000000000001196 <+32>:	mov    rax,QWORD PTR [rbp-0x10]
   0x000000000000119a <+36>:	add    rax,0x8
   0x000000000000119e <+40>:	mov    rax,QWORD PTR [rax]
   0x00000000000011a1 <+43>:	lea    rdx,[rip+0xe82]        # 0x202a
   0x00000000000011a8 <+50>:	mov    rsi,rdx
   0x00000000000011ab <+53>:	mov    rdi,rax
   0x00000000000011ae <+56>:	call   0x1040 <strcmp@plt>
   0x00000000000011b3 <+61>:	test   eax,eax
   0x00000000000011b5 <+63>:	jne    0x11cd <main+87>
   0x00000000000011b7 <+65>:	lea    rax,[rip+0xe76]        # 0x2034
   0x00000000000011be <+72>:	mov    rdi,rax
   0x00000000000011c1 <+75>:	call   0x1030 <puts@plt>
   0x00000000000011c6 <+80>:	mov    eax,0x0
   0x00000000000011cb <+85>:	jmp    0x11e6 <main+112>
   0x00000000000011cd <+87>:	lea    rax,[rip+0xe71]        # 0x2045
   0x00000000000011d4 <+94>:	mov    rdi,rax
   0x00000000000011d7 <+97>:	call   0x1030 <puts@plt>
   0x00000000000011dc <+102>:	call   0x1159 <error_message>
   0x00000000000011e1 <+107>:	mov    eax,0x0
   0x00000000000011e6 <+112>:	leave
   0x00000000000011e7 <+113>:	ret
End of assembler dump.
(gdb) disassemble error_message 
Dump of assembler code for function error_message:
   0x0000000000001159 <+0>:	push   rbp
   0x000000000000115a <+1>:	mov    rbp,rsp
   0x000000000000115d <+4>:	lea    rax,[rip+0xea4]        # 0x2008
   0x0000000000001164 <+11>:	mov    rdi,rax
   0x0000000000001167 <+14>:	call   0x1030 <puts@plt>
   0x000000000000116c <+19>:	mov    edi,0x1
   0x0000000000001171 <+24>:	call   0x1050 <exit@plt>
End of assembler dump.
```

Quiero añadir una cosa cuando veamos posibles jumps que tengamos que hacer. Cuando veamos un jmp de opcode (un salto incondicional) debemos saltar ahí en nuestro análisis directamente, no podemos hacerlo secuencial. Para los saltos condicionales, ya veremos cómo lo hacemos.

Empezamos con el prólogo de las funciones. Aquí es donde se prepara el punto de anclaje para funciones anteriores, la memoria para las variables, etc. Es nuestro stack frame. En este caso, son estas tres:

```
   0x0000000000001176 <+0>:	push   rbp
   0x0000000000001177 <+1>:	mov    rbp,rsp
   0x000000000000117a <+4>:	sub    rsp,0x10
```

push rbp esta creando el punto de anclaje para la anterior función, para, cuando terminemos esta, sepa a dónde regresar. mov es una instrucción que sirve para copiar el valor de la fuente al destino. Esto es, en sintaxis de intel, como funcionan las intrucciones:

```text
dirección de memoria <bytes al main>        operación   destino, fuente
                                            ^--------^  ^------^ ^-----^ 
                        ejemplo práctico:     mov         rbp,     rsp  
                                            
                                            Copia el valor de rsp a rbp

```

Como hemos dicho, despues de crear el punto de anclaje con el frame pointer (rbp), creamos el nuevo punto de anclaje en el marco de pila con el stack pointer, copiando el valor de rsp (stack pointer) al nuevo rbp. Luego, sub lo que hace es substraer, restar. Esto quiere decir que esta preparando el terreno para las variables que se vayan a crear. Es decir, en este caso, restamos 0x10 en hexadecimal a rsp. Cómo calculamos esto de manera rápida y sencilla? Con este comando:

```bash
echo "ibase=16; 10" | bc

Nos devuelve: 16. Con ibase le indicamos que el input es en hexadecimal (base 16). Cabe destacar que, si vamos a calcular dígitos con letras, debe ser en mayúsculas o nos dará un error de sintaxis
```

Dicho esto, sabemos que se han creado 16 bytes para las variables, ubicado en el stack pointer rsp. En este caso no lo vamos a ver, pero cuando veamos rbp-4 o rsp+4 (como ejemplo), tiene un motivo aunque la finalidad es la misma: rbp suele estar en la punta de la pila (direcciones altas) mientras que rsp suele estar en direcciones bajas. Esto es así para que tengamos claro hacia que dirección crece cada registro cuando lo veamos.

En la cuarta instrucción (+8 que equivaldría a main+8, 8 bytes adelante del main) es donde ha terminado el prólogo y empiezan las intrucciones que van a ejecutar nuestra función del main. Lo que hace esta instrucción es copiar el valor de edi a rbp-4. 

Aquí hay algo que también me parece importante matizar siendo el primer write-up, no sé si lo haré en todos (cosa que dudo), pero aquí debemos ser más exhaustivos. Es sobre las "word". Un *word*, históricamente, es el tamaño nativo de una arquitectura de CPU concreta. Es el ancho de sus registros de propósito general y de su bus de datos. Por eso el tamaño cambia según la máquina. En los primeros procesadores de 16 bits (como el 8086 original de Intel), un word eran 16 bits (2 bytes). Ahí nació el término. En arquitecturas x86-64, ese significado quedó congelado por compatibilidad histórica. Aunque hoy trabajemos con CPUs de 64 bits, en la nomenclatura de ensamblador x86, WORD sigue significando 16 bits, siempre, como reliquia del 8086 original. DWORD (double word) = 32 bits, QWORD (quad word) = 64 bits. Todos son múltiplos fijos de ese word histórico congelado, no una descripción del ancho nativo de nuestra CPU actual. La razón por la que vemos DWORD PTR (double word pointer) para un int en un binario de 64 bits es porque la etiqueta describe el tamaño del dato (int=4 bytes).

Otro matiz importante para GDB: cuando analicemos direcciones de memoria concretas y veamos una b, una h, una w o una g son convenciones de letras de la misma herramienta. b=1byte, h=halfword, 2bytes, w=word, 4bytes, g=giant, 8 bytes. Un ejemplo de comando: `x/8xb dirección_de_memoria`: la x es de examine, 8 es el tamaño a examinar, la x de hexadecimal y la b de bytes. Se traduciría como: examinar 8 bytes en hexadecimal de esta dirección de memoria. Aquí deberíamos profundizar mucho más pero nada que un poco de investigación personal no pueda solucionar (el libro de jon erikson, para esto, hace que sea muy ameno de aprender, por lo que invito al lector si le interesa cogérselo y practicarlo, es muy didáctico). 
       

Dicho esto, sigamos con el análisis. 

```text
mov DWORD PTR [rbp-0x4], edi
```

Esta es la instrucción por la que ibamos. edi, por convención (es el mismo registro que rdi pero de 32 bits) es el registro donde se pasa el primer argumento o puntero de cualquier función (tener esto en cuenta si vais a hacer ingenieria inversa, ya que identificar este tipo de patrones os ahorrarán tiempo y conexiones mentales). Otra cosa a añadir, cuando veamos unos corchetes, lo que hacemos es desreferenciar. Copiamos edi a rbp-0x4, que debe ser argc. 

Si no desreferencias -> dirección de memoria

Si desreferencias -> valor que contiene esa dirección de memoria 

Esto es otra tip a tener en cuenta. Que no os preocupe no saber explicar algo al detalle al principio, pero en esencia, es esto.

La siguiente instrucción que esta en +11, hacemos practicamente lo mismo. Copiamos el valor de rsi a rbp-0x10. Esto puede generar confusión. El anterior era dword ptr, este es qword ptr. Si pensamos en el binario, pensamos perfectamente que edi va a contener argc y rsi argv.

Luego comparamos el valor con la instrucción cmp (de compare). Estamos comparando si 2 es lo que contiene rbp-0x4. Es decir, aqui es donde se traduce el primer control de errores. Es curioso ver como compara con el dos directamente, verdad? sabiendo que en nuestro código teníamos esa redundancia y comprobabamos si argc era mayor o menor. Estas intrucciones que podemos ver ahora (de +15 a +27) cómo son traducidas. Creo que lo he comentado antes, pero si hubieramos puesto 'if (argc != 2)', sería mucho más claro. Aquí la traducción para saber si no llamamos a la función de error es compararlo con 1 (como vemos en +21), mientras que con código limpio lo compararía con 2 y habría menos "redundancias". 

La instrucción `jg` es 'jump if greater' básicamente nos esta diciendo que si es mayor de 2 argumentos, saltemos a la función de error (el main++27). Creo que antes he tenido una confusión. Para el segundo cmp que vemos, saltamos si es mayor que 1, pero si es mayor que 2, nos lleva a error message. Nada, simplemente me parece curioso ver cómo se ha traducido esto. Vayamos al salto de la linea +32, que es esta:

```text
0x0000000000001196 <+32>:	mov    rax,QWORD PTR [rbp-0x10]
```

Antes hablaba de la desreferenciación pero aquí estoy confundido. Mi confusión tenía que ver con los dobles punteros para argv. A primera vista, siendo rsi el segundo argumento, podemos pensar ahora en argv, pero no sabría decir con exactitud si es argv (dirección de memoria) o argv[0]. En teoría, copiamos el valor que contiene esa dirección de memoria a rax (que por cierto, rax es un registro que se usa para calculos matemáticos o aritméticos si no recuerdo mal. Es decir, para guardar el resultado de llamadas, que es donde creo que normalmente lo veríamos). 

En la siguiente línea que podemos ver el add, donde le estamos sumando 8 (diría que tenia que ver con el qword), lo que estamos haciendo (y lo medio recuerdo porque ya me había equivocado en esto), es sumarle 8 a argv, un puntero entero que ocupa 8 bytes. Es decir, ahora si, con esto sabemos que rax es argv[1]. 

Luego, como hemos visto con anterioridad, copiamos el valor (esta vez sí, el valor de esa dirección de memoria) de rax a rax. Esto puede parecer contraproducente o redundante, pero tiene un signficado. Lo vamos a usar para comparar el valor del segundo argumento.

```text
 0x00000000000011a1 <+43>:	lea    rdx,[rip+0xe82]        # 0x202a
```

Lo que hay después de # es la dirección de memoria ya calculada. `lea` es una instrucción (load effective address) que copia una direccion de memoria al destino (en este caso, rdx). Siendo totalmente honesto, se que tenía que ver con la dirección relativa del rip, que es un registro muy importante, rip es la versión de 64 bits, siendo la de 32 eip. Es el instrution pointer. Contiene la dirección de la siguiente instrucción a ejecutar, no la actual, y se actualia segun cuántos bytes ocupó la instrucción que se acaba de ejecutar. Vamos a comprobar esto en gdb para ver que encontramos por curiosidad:

```text
(gdb) x/8xb 0x202a
0x202a:	0x72	0x65	0x76	0x65	0x72	0x730x69	0x6e
(gdb) x/s 0x202a
0x202a:	"reversing"
```

Como podemos ver, lo que contiene esa dirección es el string hardcoded reversing. Si nos fijamos en los bytes, un ojo entrenado (copiando un poco lo que decia jon en el libro) podrá ver que esos dígitos en hexadecimal pertenecen a caracteres en ASCII.

Este análisis, para que sea justo, lo estamos haciendo sobre la marcha, asi que las investigaciones vendrán luego. Volviendo al desensamblado:

```text
   0x00000000000011a8 <+50>:	mov    rsi,rdx
   0x00000000000011ab <+53>:	mov    rdi,rax
```

Como ya hemos visto anteriormente, sabemos que ahora estamos copiando el valor de rdx a rsi, y posteriormente copiando el valor de rax a rdi. Recordamos lo que contenia rax? eso era importante. Podemos estar casi seguros que era argv[1], el primer argumento que se le pasa al binario. A veces, en cuanto los registros ya han cumplido su cometido concreto, da igual la información que tuvieran y se reutilizan, no hay problema. Simplemente copiamos rdx a rsi, que, en teoría, estamos copiando el "reversing". 

```text
0x00000000000011ae <+56>:	call   0x1040 <strcmp@plt>
```

La siguiente instrucción llama a strcmp en la dirección 0x1040, vamos a comprobar a ver que averiguamos (call es para llamadas a funciones):

```text
(gdb) x/i 0x1040
   0x1040 <strcmp@plt>:	jmp    QWORD PTR [rip+0x2fc2]        # 0x4008 <strcmp@got.plt>
(gdb) x/s 0x1040
0x1040 <strcmp@plt>:	"\377%\302/"
(gdb) x/8xb 0x1040
0x1040 <strcmp@plt>:	0xff	0x25	0xc2	0x2f	0x00	0x00	0x68	0x01
(gdb) x/8xw 0x1040
0x1040 <strcmp@plt>:	0x2fc225ff	0x01680000	0xe9000000	0xffffffd0
0x1050 <exit@plt>:	0x2fba25ff	0x02680000	0xe9000000	0xffffffc0
(gdb) x/10i 0x1040
   0x1040 <strcmp@plt>:	jmp    QWORD PTR [rip+0x2fc2]        # 0x4008 <strcmp@got.plt>
   0x1046 <strcmp@plt+6>:	push   0x1
   0x104b <strcmp@plt+11>:	jmp    0x1020
   0x1050 <exit@plt>:	jmp    QWORD PTR [rip+0x2fba]        # 0x4010 <exit@got.plt>
   0x1056 <exit@plt+6>:	push   0x2
   0x105b <exit@plt+11>:	jmp    0x1020
   0x1060 <__cxa_finalize@plt>:	jmp    QWORD PTR [rip+0x2f7a]        # 0x3fe0
   0x1066 <__cxa_finalize@plt+6>:	xchg   ax,ax
   0x1068:	Cannot access memory at address 0x1068
(gdb) x/i 0x4008
   0x4008 <strcmp@got.plt>:	rex.RX adc BYTE PTR [rax],r8b
```

Todavía no se interpretar bien la primera parte del output, parece que llama a la función en otra parte del binario. No voy a profundizar en este análisis de la línea de x/10i 0x1040. Podemos leerla pero sería complejo y nos llevará mucho rato para lo que este write-up quiere hacer. 

Nos habíamos quedado en la funcion de strcmp. Realmente si que es importante tener en cuenta lo anterior porque nos devuelve un valor, que sale aquí en el output, pero sabemos que strcmp esta comparando caracteres (en este caso, reversing hardcoded). 

```text
 0x00000000000011b3 <+61>:	test   eax,eax
```

Una línea interesante si no tenemos el valor de eax. Los test (creo) suelen comparar si es igual a 0, quizá un byte nulo... Son comprobaciones para saber cómo proceder, aunque diferente de cmp. Así que yo diría que test eax,eax va conectada con la siguiente línea, lo que nos sugiere pensar si es una comprobación de igualdad. Luego de la comprobación, tenemos esto:

```text
0x00000000000011b5 <+63>:	jne    0x11cd <main+87>
```

Como ya sabemos, jne es jump if not equal. Si no es igual esa última comprobación, saltamos a main+87, que contiene esta intrucción: `lea     rax, [rip+0xe71]`. Vamos a comprobar esa dirección de memoria a ver que contiene:

```text
(gdb) x/i 0x2045
   0x2045:	movsxd esp,DWORD PTR [r11+0x65]
(gdb) x/s 0x2045
0x2045:	"Acceso denegado"
```

No entraremos ahora en profundidad en r11, pero son registros generales que se usan para almacenar valores temporalmente. No existian en arquitecturas de 32 bits. Aquí vemos lo que contiene esa dirección de memoria, acceso denegado. Podríamos comprobar también los bytes sueltos y nos aparecería lo que hemos visto antes de la tabla ASCII, esto nos lo traduce de manera rápida. Como podemos ver en la información, estamos en un bucle, ya que estamos haciendo comprobaciones para saber si nos "concede el paso" o no. Si no es igual (básicamente la comprobación que se está haciendo con strcmp), no nos dejan pasar. En caso de que si que sea igual, tenemos esta instrucción:

```text
0x00000000000011b7 <+65>:	lea    rax,[rip+0xe76]        # 0x2034
```

Vamos a comprobarla también, a ver que nos dice. Como hemos visto antes, lea es la instrucción que guarda una dirección de memoria, en este caso en rax. A ver que nos muestra: 

```text
(gdb) x/i 0x2034
   0x2034:	movsxd esp,DWORD PTR [r11+0x65]
(gdb) x/s 0x2034
0x2034:	"Acceso concedido"
```

Sé que esta la has adivinado, pillín. Por lógica, era probable que nos mostrara esto. 

En la siguiente instrucción copiamos el valor de rax (no se si guarda la dirección de memoria, o justamente por estar entre corchetes y la desreferenciación guarda el acceso concedido, que es el valor de esa dirección de memoria, asi que voy a optar por eso) en rdi. Luego, se hace una llamada a una función (puts), que sabemos que es el printf.

Luego, copiamos el valor de 0x0 (0) a eax. Esto no se ahora mismo por qué pasa, aunque quizá lo averiguamos con las siguientes instrucciones.

Aqui tenemos un salto incondicional, (y como hemos dicho antes, debemos seguirlo):

```text
   0x00000000000011c6 <+80>:	mov    eax,0x0
   0x00000000000011cb <+85>:	jmp    0x11e6 <main+112>
```

main+112 contiene el leave, es decir, que todo ha ido correctamente. Estas dos últimas instrucciones:

```text
   0x00000000000011e6 <+112>:	leave
   0x00000000000011e7 <+113>:	ret
```

Esto es el **epílogo**, contiene el final del programa. No recuerdo exactamente qué hacia leave por detrás, era un push pop rsp pop rbp, algo por el estilo, limpiabamos la memoria con el resultado para otras funciones.

Aunque frena, que nos quedaban estas cuatro:

```text
   0x00000000000011d4 <+94>:	mov    rdi,rax
   0x00000000000011d7 <+97>:	call   0x1030 <puts@plt>
   0x00000000000011dc <+102>:	call   0x1159 <error_message>
   0x00000000000011e1 <+107>:	mov    eax,0x0
```

Copiamos el valor de rax a rdi. Luego, llama a puts (printf) y luego a error message, el típico flujo de control de errores.

Ahora nos toca la función de error_message:

```text
Dump of assembler code for function error_message:
   0x0000000000001159 <+0>:	push   rbp
   0x000000000000115a <+1>:	mov    rbp,rsp
   0x000000000000115d <+4>:	lea    rax,[rip+0xea4]        # 0x2008
   0x0000000000001164 <+11>:	mov    rdi,rax
   0x0000000000001167 <+14>:	call   0x1030 <puts@plt>
   0x000000000000116c <+19>:	mov    edi,0x1
   0x0000000000001171 <+24>:	call   0x1050 <exit@plt>
```

las primeras dos lineas son el prólogo, del que hemos hablado antes. Creamos un punto de anclaje para recordar a dónde volver con push rbp (frame pointer), a quien nos llamó (el main). Luego, creamos el nuevo punto de anclaje copiando rsp a rbp. Luego copiamos la direccion de memoria relativa que vemos a rax, la cual he analizado ya y contiene esto: 

```text
(gdb) x/i 0x2008
   0x2008:	push   rbp
(gdb) x/10i 0x2008
   0x2008:	push   rbp
   0x2009:	jae    0x206c
   0x200b:	cmp    ah,BYTE PTR gs:[eax]
   0x200f:	cmp    al,0x62
   0x2011:	imul   ebp,DWORD PTR [rsi+0x61],0x6e207972
   0x2018:	(bad)
   0x2019:	ins    DWORD PTR es:[rdi],dx
   0x201a:	gs and BYTE PTR gs:[rdi],ah
   0x201e:	jb     0x2085
   0x2020:	jbe    0x2087
(gdb) x/s 0x2008
0x2008:	"Usage: <binary name> 'reversing'."
```

En la última linea podemos ver que contiene el cómo usar el binario. Igual que antes, sé que estamos desreferenciando, asi que asumo que estamos copiando el string que contiene a rax, no una direccion de memoria. O, espera, estamos copiando esa dirección de memoria (por ser lea) que a su vez contiene ese string. 

Posteriormente copiamos rax a rdi. Esto contiene la dirección de memoria que contiene un valor. Para luego llamar a puts (donde nos imprime el usage.. Dicho esto, copiamos el valor de 0x1 a edi, y llamamos a exit. Básicamente, aunque sea lioso, esas dos útlimas líneas corresponden a exit(1) -> exit(EXIT_FAILURE).


Aquí está la nota que comentaba antes. Leer assembly "es sencillo" (y remarco el entrecomillado), pero saber abstraer esto y entender cómo era el código fuente lo considero complejo. 
