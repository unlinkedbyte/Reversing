## Crackme argc de toasterbirb

¡Buenas!

Hace tiempo que tenía este writeup pendiente. Últimamente está el repositorio un poco parado, pero tiene un motivo. No quiero seguir subiendo análisis de binarios beginner friendly (y debo decir que, actualmente, me da hasta cosa dejar los dos primeros análisis que subí al repositorio jaja, el exercise-1-with-code y el segundo sin). La cosa es que he estado montándome un laboratorio para hardware hacking para los fines de semana (sí, únicamente para los fines de semana, en teoría. No es lo que quisiera, pero la ruta de estudio la tengo más o menos bien definida y, además, se alimentan mutuamente). Entre semana, por el momento, estoy intentando mejorar en mi programación en C y, actualmente, aprender el formato ELF (estoy profundizando en ello). Este writeup lo he cogido con más ganas por ese motivo; aunque no he terminado el segundo capítulo todavía, ahora tengo una mejor comprensión del formato ELF, de las secciones y segmentos (aunque no es algo que se aprenda en una semana debo decir). 

Como este no va a ser un writeup explicando el formato ELF (aunque quizá sí explique alguna cosa que en el momento sea necesaria), recomiendo firmemente al que esté empezando en reversing echarle una ojeada, pues todos los binarios de Linux y la mayoría de Unix modernos usan este formato. Dicho esto, el libro que recomiendo para ello es `Practical Binary Analysis` de Dennis Andriesse. 

Este crackme tenía ganas de subirlo porque ocurre algo que, cuando lo vi, no sabía que fuera posible: el desensamblado nos dice una cosa pero luego ocurre otra. Luego veremos cómo llegamos ahí. Por el momento, este es el crackme por si alguien quisiera hacérselo: [argc de toasterbirb](https://crackmes.one/crackme/68698837aadb6eeafb399017).

*(Nota: al final ha habido más tecnicismos de los que quisiera. Si no te apetece leerlo todo, puedes ir simplemente al título donde pone "Por qué main nos miente..." o quizá un poco antes donde puedas ver el análisis del desensamblado para que sepas cómo llego a esa pregunta)

### Estructura

1. 

Como siempre hacemos, vamos a empezar con la típica estructura a la hora de analizar un binario. A readelf recurriríamos realmente cuando la cosa se pone compleja, en análisis de malware, o cuando queramos comprobar algo "a mano", pero ni siquiera sería necesario para binarios de este tipo. 

Empezamos con el comando file:

```bash
file argc 
argc: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 3.2.0, with debug_info, not stripped
```

Como ya sabemos (y esto sí que voy a volver a hacerlo porque me sirve de repaso para mí mismo) estos son los campos que podemos apreciar: es un ELF de 64 bits (aquí realmente habla de la estructura interna del ELF, no de la arquitectura del procesador, aunque vayan ligados), LSB es little endian (least significant byte first) que coloca primero el byte menos significativo de cada valor numérico (y las cadenas no se ven afectadas porque no son un número, sino una sucesión de bytes). 

Es un pie executable (recordemos, es la propiedad del binario que permite al kernel ejecutar el ASLR sobre el binario), la arquitectura es x86-64 (x86-64 la ISA, creada por AMD y adoptada más tarde por Intel), la ABI es el siguiente campo con la versión (aunque estrictamente hablando, en el header del binario, son 3 campos distintos del e_ident. Lo puedes comprobar en el libro que te he recomendado al principio, en las fuentes oficiales, o en el header file de tu sistema /usr/include/elf.h), luego vemos que está dinámicamente enlazado; y el intérprete, que es el enlazador dinámico (linker).  

Vemos la versión mínima del kernel donde puede ejecutarse el binario (lo podemos comprobar en la .note.ABI-tag). Vemos que tiene debug_info (significa que están las secciones DWARF. DWARF no significa nada como tal, es un nombre que acogieron por tener "relación" con ELF, a modo de criaturas mitológicas), lo que nos permitiría ver tipos, nombres de variables locales, firmas y correspondencia con números de línea del fuente. Y que no está stripped (conserva símbolos, es decir, nombres de funciones. Hace más fácil el análisis pero no es lo habitual). 


2.

Le pasamos el comando strings ahora para una primera pasada y hacernos una idea de lo que nos podemos encontrar:

```bash
strings argc

/lib64/ld-linux-x86-64.so.2
*** puts
__libc_start_main
__cxa_finalize
*** strcmp
libc.so.6
GLIBC_2.2.5
GLIBC_2.34
_ITM_deregisterTMCloneTable
__gmon_start__
_ITM_registerTMCloneTable
PTE1
*** please try again and make sure to give the correct amount of arguments (
*** correct! (
*** wrong passwords...
9*3$"
GCC: (Gentoo 14.3.0 p8) 14.3.0
.B#>M$#=1
LLu=/
../sysdeps/x86_64/start.S
*** /var/tmp/portage/sys-libs/glibc-2.40-r11/work/glibc-2.40/csu
GNU AS 2.44.0
_start
__abi_tag
n_type
Elf64_Nhdr
__int128
__uint32_t
Elf64_Word
name
long long unsigned int
*** GNU C11 14.3.0 -m64 -march=znver3 -ggdb -ggdb -O2 -O2 -std=gnu11 -fgnu89-inline -fmerge-all-constants -frounding-math -fstack-protector-strong -fno-common -fmath-errno -fPIE -fcf-protection=full -ftls-model=initial-exec -foffload-options=-fno-stack-protector -foffload-options=-fcf-protection=none
unsigned char
n_descsz
_Bool
n_namesz
long double
short unsigned int
nhdr
__int32_t
desc
float
short int
long long int
_IO_stdin_used
../sysdeps/x86_64/crti.S
../sysdeps/x86_64/crtn.S
/var/tmp/portage/sys-libs/glibc-2.40-r11/work/glibc-2.40/csu
../sysdeps/x86_64
start.S
../sysdeps/x86/abi-note.c
../sysdeps/x86
../posix/bits
../bits
../elf
../csu
types.h
stdint-intn.h
stdint-uintn.h
elf.h
init.c
crti.S
crtn.S
abi-note.c
__abi_tag
init.c
main.c
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
__bss_start
*** main
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
.debug_aranges
.debug_info
.debug_abbrev
.debug_line
.debug_str
.debug_line_str
.debug_rnglists
```

He puesto tres asteriscos ya que, en este caso (debido también al debug_info), tenemos un output grande. En pos de la brevedad nos vamos a centrar en los que he marcado y hacemos una explicación rápida. Aunque antes de nada, añadir que las líneas precedidas por un punto son las secciones, y las precedidas por una barra baja o dos es la convención de nombres reservados de C. 

Primero de todo podemos ver un par de funciones de librerías de C, puts y strcmp. Cuando veamos un puts, típicamente, podemos pensar en un printf que no lleva ningún especificador de formato y termina en \n (puts lo añade automáticamente). Dejo las funciones que, aun haciendo strip, no desaparecerían:

```bash
nm -D argc
                 w __cxa_finalize@GLIBC_2.2.5
                 w __gmon_start__
                 w _ITM_deregisterTMCloneTable
                 w _ITM_registerTMCloneTable
                 U __libc_start_main@GLIBC_2.34
                 U puts@GLIBC_2.2.5
                 U strcmp@GLIBC_2.2.5
```

Luego he marcado los 3 constant strings que están hardcodeados en el código fuente, que simplemente nos dan algo más de información. Para crackmes pueden servirnos, para software real, dudo que sea sencillo. Dejo también la sección .rodata aquí plasmada:

```bash
objdump -d -M intel -j .rodata argc 

argc:     formato del fichero elf64-x86-64


Desensamblado de la sección .rodata:

0000000000002000 <_IO_stdin_used>:
    2000:	01 00 02 00 00 00 00 00 70 6c 65 61 73 65 20 74     ........please t
    2010:	72 79 20 61 67 61 69 6e 20 61 6e 64 20 6d 61 6b     ry again and mak
    2020:	65 20 73 75 72 65 20 74 6f 20 67 69 76 65 20 74     e sure to give t
    2030:	68 65 20 63 6f 72 72 65 63 74 20 61 6d 6f 75 6e     he correct amoun
    2040:	74 20 6f 66 20 61 72 67 75 6d 65 6e 74 73 20 28     t of arguments (
    2050:	e2 80 9e e1 b5 95 e1 b4 97 e1 b5 95 e2 80 9e 29     ...............)
    2060:	00 63 6f 72 72 65 63 74 21 20 28 cb b6 e1 b5 94     .correct! (.....
    2070:	20 e1 b5 95 20 e1 b5 94 cb b6 29 00 77 72 6f 6e      ... .....).wron
    2080:	67 20 70 61 73 73 77 6f 72 64 73 2e 2e 2e 00        g passwords....
```

Luego, pese a que no lo he marcado, podemos ver Gentoo (que es una distribución de Linux) y la versión del compilador. Menciono esto porque los dos siguientes campos (`/var/tmp/portage/sys-libs/glibc-2.40-r11/work/glibc-2.40/csu` y `GNU C11 14.3.0 -m64 -march=znver3 -ggdb -ggdb -O2 -O2 -std=gnu11 -fgnu89-inline -fmerge-all-constants -frounding-math -fstack-protector-strong -fno-common -fmath-errno -fPIE -fcf-protection=full -ftls-model=initial-exec -foffload-options=-fno-stack-protector -foffload-options=-fcf-protection=none`) nos dan cierta información. Lo primero es el directorio donde Gentoo compila glibc, y lo segundo son flags con las que se compiló glibc. 

Y poco más, podemos ver tipos y demás gracias al debug_info y la función del main. 


3.

Ahora nos toca readelf. No los pondré todos (me refiero a todas las flags posibles), pero algo que he estado estudiando hoy creo que sí merece la pena ponerlo (puedes saltarte esta parte sin problema):

```bash
readelf -h argc

Encabezado ELF:
 1) Mágico:  7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Clase:                             ELF64
  Datos:                             complemento a 2, little endian
 2) Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  Versión ABI:                       0
 3) Tipo:                              DYN (Position-Independent Executable file)
 4) Máquina:                           Advanced Micro Devices X86-64
 5) Versión:                           0x1
 6) Dirección del punto de entrada:    0x10e0
  Inicio de encabezados de programa: 64  (bytes en el fichero)
  Inicio de encabezados de sección:  16240 (bytes en el fichero)
 7) Opciones:                          0x0
 8) Size of this header:               64 (bytes)
 9) Size of program headers:           56 (bytes)
  Number of program headers:         13
  Size of section headers:           64 (bytes)
  Number of section headers:         37
  Section header string table index: 36
```

Primero el header (corresponde al número mágico, lo del e_ident que mencionaba marcado como 1) en el output de arriba, que está en el header file /usr/include/elf.h y podemos mirar para ser más específicos). Siempre empieza por 0x7f. Los siguientes 3 bytes son ELF en la tabla ASCII, el siguiente es EI_CLASS que nos indica la clase (01 es ELFCLASS32, 02 es ELFCLASS64. Este es el campo que vemos al principio del file después del ELF).

Después vemos un 01 que es ELFDATA2LSB y nos indica el endianness, y el último 01 es EV_CURRENT que es la versión (marcada como 2. Este campo por el momento solo puede ser 01). Los bytes siguientes están todos a cero. Los dos siguientes al 1 son EI_OSABI y EI_ABIVERSION y están así ya que están en su valor por defecto, los bytes 9 al 15 son siempre 0 por el momento, no tienen un uso definido todavía. 

Cuando termina el array e_ident (véase el header file del sistema, son 16 bytes) viene un conjunto de campos de íntegros multibyte. Donde está el número 3, que es el primero de estos, se llama e_type, y especifica el tipo del binario. Los valores más comunes que encontraremos son ET_REL (indicándonos un object file, siempre son reubicables), ET_EXEC (un binario ejecutable), ET_DYN (una librería dinámica, también llamada shared object file). 

Para el número 4 tenemos el campo e_machine, que resalta la arquitectura en la que el binario está destinado a ejecutarse.

Para el número 5 tenemos el campo e_version que tiene el mismo rol que el byte de e_ident. Específicamente, nos indica la versión de la especificación ELF que fue usada cuando se creó el binario.

Luego, en el número 6 tenemos el entry point, que es el campo e_entry del struct del header file. Es la dirección virtual en la cual la ejecución debe empezar. Aquí es donde el intérprete (normalmente ld-linux.so) transferirá el control después de finalizar el loading del binario en la memoria virtual. Es útil para desensamblar recursivamente, ya que estamos. Fijémonos en ella, que volveremos luego.

El campo que he marcado como 7 son las flags. Nos puede dar más información en sistemas embebidos ya que nos puede dar detalles adicionales sobre la interfaz que espera del sistema operativo embebido. Para binarios x86, e_flags normalmente está seteado a cero y, por lo tanto, no es de nuestro interés aquí.

En el número 8 tenemos el campo e_ehsize (eh de elf header), especifica el tamaño del header ejecutable, en bytes. Para binarios x86-64, el tamaño del ejecutable header siempre es 64 bytes, mientras que en arquitecturas de 32 bits es 52. 

Para el 9, este último campo es algo más complejo de explicar con palabras propias. Tanto el 9 como los 4 siguientes van ligados. Aquí nos indica que el tamaño de los program headers es de 56 bytes y que tiene 13, por lo tanto, tenemos 13 program headers de 56 bytes cada uno. La misma lógica es aplicable a las dos siguientes líneas. 

### Empezando por gdb 

Esta vez vamos a abrir gdb directamente, luego entenderemos el por qué. Cabe destacar que no debes ejecutar un binario con gdb de cuyo origen no te fíes. Siempre asegúrate de ejecutar las herramientas pertinentes en el análisis estático y comprobar los permisos de las secciones o segmentos. Si ves una rwx o que sea ejecutable y escribible a la vez, eso ya sería causa de preocupación, extraño cuanto menos, ya que eso no lo ha hecho el compilador. Puedes usar readelf -SW para comprobar las secciones y readelf -lW para segmentos (que es lo que importa en ejecución).

Yo abriré gdb directamente por los análisis previos y porque es un crackme con autor conocido. Así que empezamos:

```text
(gdb) disassemble main
Dump of assembler code for function main:
   0x0000000000001080 <+0>:	    endbr64
   0x0000000000001084 <+4>:	    push   rbx
   0x0000000000001085 <+5>:	    cmp    edi,0x3
   0x0000000000001088 <+8>: 	jne    0x10c3 <main+67>
   0x000000000000108a <+10>:	mov    rax,QWORD PTR [rsi+0x10]
   0x000000000000108e <+14>:	mov    rdi,QWORD PTR [rsi+0x8]
   0x0000000000001092 <+18>:	mov    rsi,rax
   0x0000000000001095 <+21>:	call   0x1070 <strcmp@plt>
   0x000000000000109a <+26>:	mov    ebx,eax
   0x000000000000109c <+28>:	test   eax,eax
   0x000000000000109e <+30>:	je     0x10b5 <main+53>
   0x00000000000010a0 <+32>:	lea    rdi,[rip+0xfd5]        # 0x207c
   0x00000000000010a7 <+39>:	call   0x1060 <puts@plt>
   0x00000000000010ac <+44>:	mov    ebx,0x1
   0x00000000000010b1 <+49>:	mov    eax,ebx
   0x00000000000010b3 <+51>:	pop    rbx
   0x00000000000010b4 <+52>:	ret
   0x00000000000010b5 <+53>:	lea    rdi,[rip+0xfa5]        # 0x2061
   0x00000000000010bc <+60>:	call   0x1060 <puts@plt>
   0x00000000000010c1 <+65>:	jmp    0x10b1 <main+49>
   0x00000000000010c3 <+67>:	lea    rdi,[rip+0xf3e]        # 0x2008
   0x00000000000010ca <+74>:	call   0x1060 <puts@plt>
   0x00000000000010cf <+79>:	jmp    0x10ac <main+44>
End of assembler dump.
```

La sección de .rodata:

```text
(gdb) info file
  ...
  
  0x0000000000002000 - 0x000000000000208f is .rodata
  
  ...
```

Y pongo también todos los strings para que sepamos a qué nos referimos en cada zona: 

```text
(gdb) x/s 0x207c
0x207c:	"wrong passwords..."
(gdb) x/s 0x2061
0x2061:	"correct! (˶ᵔ ᵕ ᵔ˶)"
(gdb) x/s 0x2008
0x2008:	"please try again and make sure to give the correct amount of arguments („ᵕᴗᵕ„)"
```

Vamos ahora al análisis de main, para entender qué hace. Adelanto que aquí empieza, pero lo interesante se pone luego, algo más allá. 

Teniendo esto, al no ver el típico prólogo que hemos visto hasta ahora donde guardamos el frame pointer o base pointer `rbp` en el stack, que es el frame pointer de la función que nos llamó para luego crear este nuevo punto de anclaje a partir del stack pointer `rsp` (el típico push rbp, mov rbp,rsp) para luego restar cierta cantidad de bytes que es la memoria que estaríamos reservando para esa función, ya vemos que hay algún tipo de optimización (lo que no puedo asegurar con certeza cuál. Probablemente -O2 o -O3 a la hora de compilar).

Empezamos con la instrucción `endbr64`, la cual he explicado en otro writeup (lo enlazo al final por si quisieras algo más de detalle) pero básicamente es parte de CET, una protección de Intel contra ROP y JOP. Un salto o llamada indirecta solo puede aterrizar en una instrucción endbr64. Si el procesador salta a cualquier otro sitio por vía indirecta, lanzará una excepción. Esa instrucción es lo que evita ese tipo de ataques. Y la menciono porque es importante para algo que veremos luego. [En este writeup puedes ver una explicación, quizá, mejor](https://github.com/unlinkedbyte/Reversing/tree/main/crackmes/u-cant-pass).

Luego tenemos el push rbx que lo podríamos considerar el prólogo. `rbx` es un registro callee-saved. La convención de llamada de x86-64 reparte los registros en dos grupos:

* **Volátiles:** que cualquier función puede destruir: rax, rcx, rdx, rsi, rdi, r8-r11.

* **Preservados:** que si los usas tienes que devolverlos como estaban: rbx, rbp, r12-r15.

Dicho esto, lo primero que vemos después de ese prólogo es la comparación de edi a 0x3. Es decir, edi es el registro de 32 bits para el primer argumento de (en este caso) nuestro main. Es el típico control de errores if (argc != 3). Así que lo que deducimos de aquí es que el binario necesita 3 argumentos (con el nombre del binario incluido, claro está, argv[0]). En caso de no ser igual (por la siguiente instrucción, jne -> jump if not equal) nos iríamos a main+67, que carga la dirección de memoria de 0x2008 en rdi (como primer argumento para la siguiente llamada), que contiene el string "please try again and make sure to give the correct amount of arguments („ᵕᴗᵕ„)", llama a puts (que nos imprime por pantalla este error o pseudo error), nos vamos a main+44 por el salto incondicional del final, y entraríamos en lo que es el epílogo (que está en medio del desensamblado). Este epílogo lo que hace es copiar 0x1 en ebx, luego copiar ebx en eax, sacar rbx de la pila y hacer el return para volver a la función que nos llamó. El mov ebx,0x1 nos hace entender un return 1 (error).

En caso de ser igual a 3 argumentos, (volviendo al jne de la cuarta instrucción) simplemente seguiríamos el curso normal del programa. Tenemos ahora estas 4 instrucciones que las pongo para situarnos mejor, por si el que estuviera leyendo esto está empezando: 

```text
   0x000000000000108a <+10>:	mov    rax,QWORD PTR [rsi+0x10]
   0x000000000000108e <+14>:	mov    rdi,QWORD PTR [rsi+0x8]
   0x0000000000001092 <+18>:	mov    rsi,rax
   0x0000000000001095 <+21>:	call   0x1070 <strcmp@plt>
```

Aquí sucede algo interesante. Aún habiendo optimizaciones de por medio parece que hay una pequeña redundancia, lo cual es un gasto de instrucciones y tiempo (independientemente de que sea mínimo en el rendimiento posterior). La primera instrucción está copiando argv[2] en rax (porque si nos fijamos en la tercera instrucción, luego lo copia a rsi, que es el registro para el segundo argumento de una función). ¿Por qué? Simplemente parece que el compilador, en el caso de la primera instrucción de ese bloque, está usando un registro intermedio que no era estrictamente necesario, corregidme si me equivoco o si alguien tiene la respuesta real. Pero podría haberse resumido.

**¿Por qué vemos que es argv[1] y argv[2]?**

Es por el tamaño del operando. QWORD PTR son 8 bytes. Para desplazarnos por argv utilizamos punteros, que en arquitecturas de 64 bits, son de 8 bytes. Dicho esto, es para preparar la llamada a strcmp, que lo que compara simplemente es si argv[1] es igual a argv[2]. 

Luego, después de la llamada a strcmp, tenemos estas líneas: 

```text
   0x000000000000109a <+26>:	mov    ebx,eax
   0x000000000000109c <+28>:	test   eax,eax
   0x000000000000109e <+30>:	je     0x10b5 <main+53>
```

Guardamos el resultado que devuelva strcmp (0 si es igual, diferente a 0 si no) en ebx (el resultado primero es guardado en eax como vemos). Y luego, comprobamos mediante la instrucción test, que hace un AND (&) a nivel de bits de eax consigo mismo y tira el resultado, quedándose con los flags. Es la forma de comprobar si es 0 el resultado. Si eax vale 0 se activa el flag ZF, nos vamos a main+53, que nos imprime el correct y podemos ya salir, si no es igual, nos dará wrong passwords. 


Ahora mismo ya tenemos el binario mapeado (lo que esperamos de él, o lo que él espera recibir de nosotros). Así que vamos a comprobarlo: 

```bash
$./argc test test
please try again and make sure to give the correct amount of arguments („ᵕᴗᵕ„)

./argc test testing
please try again and make sure to give the correct amount of arguments („ᵕᴗᵕ„)
```

**¿Por qué esta respuesta?**

Esto es lo que vamos a averiguar. Primero vamos a comprobar empíricamente a ver si "se rompe" u ocurre algo distinto:

```bash
$./argc test test test
please try again and make sure to give the correct amount of arguments („ᵕᴗᵕ„)

./argc test test test test
please try again and make sure to give the correct amount of arguments („ᵕᴗᵕ„)

./argc test test test test test
correct! (˶ᵔ ᵕ ᵔ˶)

./argc test test test test test test
correct! (˶ᵔ ᵕ ᵔ˶)

./argc test test test test test test test
please try again and make sure to give the correct amount of arguments („ᵕᴗᵕ„)
```

Vale, como podemos ver, cuando metemos 6 o 7 argumentos (el nombre del binario incluido) nos da correcto. ¿Por qué nos miente el main?

Antes de indagar, vamos a hacer otra comprobación rápida:

```bash
./argc test testing test test test
wrong passwords...
```

Aunque hemos pasado los argumentos correctos, vemos que lo demás del desensamblado está bien. Comprueba si los dos primeros argumentos (test y testing en este caso) son iguales. 

Vayamos ahora a perseguir y entender por qué ocurre esto. Primero de todo, ¿qué deberíamos hacer? Vamos a comprobar qué contiene edi:

```text
gdb -q ./argc
Reading symbols from ./argc...
(gdb) break main
Breakpoint 1 at 0x1080
(gdb) run test test test test test
Starting program: /home/ygm/crackmes/argc-toasterbirb/argc test test test test test
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".

Breakpoint 1, 0x0000555555555080 in main ()
(gdb) i r edi
edi            0x3                 3
```

¿Por qué nos devuelve 3 si hemos metido 5 (6 en total)? Por ende, si ejecutamos run test test, nos debería devolver 1:

```text
gdb -q ./argc
Reading symbols from ./argc...
(gdb) break main
Breakpoint 1 at 0x1080
(gdb) run test test
Starting program: /home/ygm/crackmes/argc-toasterbirb/argc test test
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".

Breakpoint 1, 0x0000555555555080 in main ()
(gdb) i r edi
edi            0x1                 1
```

¿Y qué contiene argv en el caso de los 6 argumentos (volviendo a cuando hemos ejecutado run + 5 test)?

```text
(gdb) i r edi
edi            0x3                 3
(gdb) x/3gx $rsi
0x7fffffffdca8:	0x00007fffffffe077	0x00007fffffffe0a0
0x7fffffffdcb8:	0x00007fffffffe0a5
(gdb) x/s 0x00007fffffffe077
0x7fffffffe077:	"/home/ygm/crackmes/argc-toasterbirb/argc" <-- nombre del binario
(gdb) x/s 0x00007fffffffe0a0
0x7fffffffe0a0:	"test"  <--- primer argumento
(gdb) x/s 0x00007fffffffe0a5
0x7fffffffe0a5:	"test" <---- segundo argumento
```

### ¿Por qué main nos miente? ¿O le están mintiendo a main? Lo perseguimos hasta encontrar la respuesta

Antes de nada debemos saber algo. A main lo llama `__libc_start_main` (una función de libc), y a `__libc_start_main` lo llama `_start`. Aquí te dejo un writeup que quizá te sirva también: [stripped-binary](https://github.com/unlinkedbyte/Reversing/blob/main/binary-analysis/stripped-binary/README.md).

¿Qué es libc?

libc es la biblioteca estándar de C (donde vive printf, strcmp, malloc...). El binario no lleva el código dentro, lo toma prestado de libc.so.6 en tiempo de ejecución. Aunque cabe destacar que libc hace algo más que prestar funciones, también arranca el programa. Antes de que main corra, alguien tiene que montar el entorno (streams de entrada y salida, variables de entorno, registrar la limpieza al salir...) y de eso se encarga `__libc_start_main`. Y, para poder llamar a main(argc, argv), `__libc_start_main` tiene que recibir argc de alguien, que se lo pasa quien la llama, y es `_start`. 

En el writeup para encontrar el main en binarios stripped lo verás mucho más claro. 

Sabiendo cómo funciona y quién llama a quién, y qué se ejecuta antes del main, tenemos que ir directamente a `_start`. Ya que libc.so.6 es un fichero enorme y compartido por todo el sistema. Si el autor hubiera modificado libc, el crackme solo funcionaría en su máquina. Y, además (como ya hemos dicho), `__libc_start_main` recibe argc y se lo pasa a main, es decir, que el valor ya "venía mal". 

Dicho esto, veamos que sucede aquí:

```text
 disassemble _start
Dump of assembler code for function _start:
   0x00000000000010e0 <+0>:	    pop    rax
   0x00000000000010e1 <+1>:	    shr    al,1
   0x00000000000010e3 <+3>:	    push   rax
   0x00000000000010e4 <+4>:	    xor    ebp,ebp
   0x00000000000010e6 <+6>:	    mov    r9,rdx
   0x00000000000010e9 <+9>:	    pop    rsi
   0x00000000000010ea <+10>:	mov    rdx,rsp
   0x00000000000010ed <+13>:	and    rsp,0xfffffffffffffff0
   0x00000000000010f1 <+17>:	push   rax
   0x00000000000010f2 <+18>:	push   rsp
   0x00000000000010f3 <+19>:	xor    r8d,r8d
   0x00000000000010f6 <+22>:	xor    ecx,ecx
   0x00000000000010f8 <+24>:	lea    rdi,[rip+0xffffffffffffff81]        # 0x1080 <main>
   0x00000000000010ff <+31>:	call   QWORD PTR [rip+0x2ed3]        # 0x3fd8
   0x0000000000001105 <+37>:	hlt
```

En la primera línea, en la dirección de memoria virtual, tenemos el entry point que comentaba al principio. Dicho esto, ¿ves algo raro aquí? Si has empezado hace poco en reversing, como explicaba en el writeup del binario stripped, probablemente no hayas mirado más funciones que las que tenía el código fuente y van a parar a la sección .text . No pasa nada, es normal. Lo que podemos hacer es compararlo con un binario normal compilado por mí y ver qué main nos devuelve (lo haremos con un par, para descartar posibles falsos positivos):

```text
gdb -q ./bucle
Reading symbols from ./bucle...
(No debugging symbols found in ./bucle)
(gdb) disassemble _start 
Dump of assembler code for function _start:
   0x0000000000001070 <+0>:	    xor    ebp,ebp
   0x0000000000001072 <+2>:	    mov    r9,rdx
   0x0000000000001075 <+5>:	    pop    rsi
   0x0000000000001076 <+6>:	    mov    rdx,rsp
   0x0000000000001079 <+9>:	    and    rsp,0xfffffffffffffff0
   0x000000000000107d <+13>:	push   rax
   0x000000000000107e <+14>:	push   rsp
   0x000000000000107f <+15>:	xor    r8d,r8d
   0x0000000000001082 <+18>:	xor    ecx,ecx
   0x0000000000001084 <+20>:	lea    rdi,[rip+0xff]        # 0x118a <main>
   0x000000000000108b <+27>:	call   QWORD PTR [rip+0x2f2f]        # 0x3fc0
   0x0000000000001091 <+33>:	hlt
End of assembler dump.
```

Quiero dejar el desensamblado con gdb, pero creo que es mucho más visual el de objdump, ya que queda más claro en la única llamada que se realiza: 

```text
objdump -d -M intel -j .text bucle

bucle:     formato del fichero elf64-x86-64


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
    1084:	48 8d 3d ff 00 00 00 	lea    rdi,[rip+0xff]        # 118a <main>
    108b:	ff 15 2f 2f 00 00    	call   QWORD PTR [rip+0x2f2f]        # 3fc0 <__libc_start_main@GLIBC_2.34>
    1091:	f4                   	hlt
    1092:	66 2e 0f 1f 84 00 00 	cs nop WORD PTR [rax+rax*1+0x0]
    1099:	00 00 00 
    109c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
```

Ahora bien, si estuviéramos en un binario stripped, los símbolos se han borrado, por lo que no puedes usar `disassemble _start`. Entonces nada, simplemente desensamblas a partir del entry point (pongo un ejemplo con un binario distinto, stripped):

```text
gdb -q ./clave
Reading symbols from ./clave...
(No debugging symbols found in ./clave)
(gdb) disassemble _start
No symbol table is loaded.  Use the "file" command.
(gdb) x/12i 0x10c0
   0x10c0:	xor    ebp,ebp
   0x10c2:	mov    r9,rdx
   0x10c5:	pop    rsi
   0x10c6:	mov    rdx,rsp
   0x10c9:	and    rsp,0xfffffffffffffff0
   0x10cd:	push   rax
   0x10ce:	push   rsp
   0x10cf:	xor    r8d,r8d
   0x10d2:	xor    ecx,ecx
   0x10d4:	lea    rdi,[rip+0xffffffffffffff95]        # 0x1070
   0x10db:	call   QWORD PTR [rip+0x2edf]        # 0x3fc0
   0x10e1:	hlt
```

Sé que puede ser un poco lioso. Básicamente, el `lea` anterior a la `call`, carga la dirección de memoria de main en rdi, y rdi es el registro para el primer argumento, luego llama a `__libc_start_main` pasándole el primer argumento (la dirección de main) y ya estaría, resumidamente. Dejando eso de lado un momento, voy a volver a poner el output del desensamblado de `_start` del binario que nos toca para que no tengas que subir arriba y a partir de ahí analizamos:

```text
gdb -q ./argc
Reading symbols from ./argc...
(gdb) disassemble _start
Dump of assembler code for function _start:
   0x00000000000010e0 <+0>:	    pop    rax
   0x00000000000010e1 <+1>:	    shr    al,1
   0x00000000000010e3 <+3>:	    push   rax
   0x00000000000010e4 <+4>:	    xor    ebp,ebp
   0x00000000000010e6 <+6>:	    mov    r9,rdx
   0x00000000000010e9 <+9>:	    pop    rsi
   0x00000000000010ea <+10>:	mov    rdx,rsp
   0x00000000000010ed <+13>:	and    rsp,0xfffffffffffffff0
   0x00000000000010f1 <+17>:	push   rax
   0x00000000000010f2 <+18>:	push   rsp
   0x00000000000010f3 <+19>:	xor    r8d,r8d
   0x00000000000010f6 <+22>:	xor    ecx,ecx
   0x00000000000010f8 <+24>:	lea    rdi,[rip+0xffffffffffffff81]        # 0x1080 <main>
   0x00000000000010ff <+31>:	call   QWORD PTR [rip+0x2ed3]        # 0x3fd8
   0x0000000000001105 <+37>:	hlt
End of assembler dump.
```

¿Qué podemos apreciar aquí? Parece que hay 3 instrucciones nuevas. En este caso, quita rax de la pila,  usa la instrucción shr que es shift right (desplaza los bits del operando hacia la derecha tantas posiciones como le digas y los huecos que quedan en la izquierda se rellenan con ceros. El bit que sale por la derecha se pierde o acaba en el carry flag). Lo que significa shr al,1 en este contexto es que está desplazando un bit a la derecha de al, que lo que quiere decir es que está dividiendo entre 2, y luego vuelve a meter rax en la pila. `al`, cabe destacar, son los 8 bits bajos de rax. 

Pues aquí tenemos el problema resuelto. Sabemos que argc se divide entre 2. ¿Y qué número dividido entre 2 da 3? 6. Aunque hay algo más, como las divisiones son divisiones enteras y truncan, ¡7 también es un resultado válido!

¿Os acordáis que había hecho especial mención a la instrucción endbr64? Lo resolvemos aquí. La modificación del `_start` podría haberse hecho programando en ensamblador (lo que, sinceramente, me parece una locura jaja), o puede haber pasado lo que vamos a explicar.

`endbr64` es una instrucción que ocupa 4 bytes. Lo más probable es que parcheara el binario, ya que en el desensamblado estático (que no he enseñado) estaba presente en las demás funciones. Por eso hay 3 instrucciones nuevas. Mira, lo pongo aquí mejor:

```text
endbr64   = f3 0f 1e fa ---> 4 bytes
pop rax   = 58          ---> 1 byte
shr al,1  = d0 e8       ---> 2 bytes
push rax  = 50          ---> 1 byte
```
Esas 3 instrucciones suman 4 bytes, el reemplazo del endbr64 que ocupa 4. 

Por cierto, en los binarios que yo he mostrado propios no estaban compilados con la flag que activa la protección contra ROP y JOP (el endbr64), pero en este sí. Así que un binario compilado con esa protección, o todas las funciones susceptibles de ser destino indirecto llevan esa instrucción o no la lleva ninguna. Un binario donde `main` la tiene y `_start` no es una inconsistencia apreciable. 

### Sección añadida: lo parcheamos de vuelta

Después de haberlo subido y haberlo vuelto a leer, no me acababa de convencer el final. Así que me he preguntado: ¿Y si lo parcheo de vuelta para comprobar si las afirmaciones hechas son ciertas?

Primero de todo creamos una copia del binario para no modificar el que original, por si algo sale mal: 
```bash
$cp argc ./argc_patched

$ls -l
.rw-rw-r-- ygm ygm 3.6 KB Thu Aug  6 22:59:50 2026  68698837aadb6eeafb399017.zip
.rwx------ ygm ygm  18 KB Sat Jul  5 22:16:55 2025  argc
.rwx------ ygm ygm  18 KB Thu Aug 27 20:46:22 2026  argc_patched
```

Primero de todo abrimos el binario con el comando `nvim -b argc_patched` para abrirlo con nvim en modo binario. Y, estando dentro, usamos este comando para ver los datos ilegibles en hex: `:%!xxd`.

Luego filtramos por el entry point que teníamos para averiguar donde está `_start`. Una vez encontrado, podemos ver esta línea:

```text
000010e0: 58d0 e850 31ed 4989 d15e 4889 e248 83e4  X..P1.I..^H..H.. 
```

Empieza directamente al principio, así que no tenemos que buscar demasiado. Genial. Ahora lo que debemos hacer es cambiar esos 4 primeros bytes por la instrucción endbr64, que la he dejado arriba. Ahora, la misma secuencia debe quedarnos así:

```text
000010e0: f30f 1efa 31ed 4989 d15e 4889 e248 83e4  X..P1.I..^H..H..
```

Ahora guardamos los cambios así, primero guardando los cambios que hemos hecho con xxd: `:%!xxd -r` y luego saliendo normal con `:wq`.

Una vez fuera, vamos a comprobar el resultado con varias opciones:

```bash
./argc_patched test test
correct! (˶ᵔ ᵕ ᵔ˶)

./argc_patched test test test test test
please try again and make sure to give the correct amount of arguments („ᵕᴗᵕ„)

./argc_patched test test test test test test
please try again and make sure to give the correct amount of arguments („ᵕᴗᵕ„)

./argc_patched test test test 
please try again and make sure to give the correct amount of arguments („ᵕᴗᵕ„)
```

¡Y aquí lo tenemos! Da gusto cuando las cosas salen bien, aunque este binario fuera beginner friendly también. Bueno, un writeup pendiente menos, espero que hayas podido aprender mucho.
