## Cómo identificar el main en un binario stripped con PIE

Buenas. El otro día estaba haciendo algún que otro crackme y me encontré uno que era de dificultad baja pero que luego, para sorpresa mía, vi que estaba stripped. Me pareció un buen momento para empezar a entender cómo funcionarían los binarios de software del mundo real, ya que además, tenía optimizaciones -O2. Dicho esto, no será un análisis exhaustivo sobre cómo resolver un crackme (que ya está resuelto y replicado en el código que dejaré en el mismo directorio, con una clave distinta), lo que vamos a ver (como el título indica) es cómo aprender a identificar el main en un binario que no tenga símbolos.

*Nota: además, verás una sección sobre cómo GDB trata el ASLR y por qué la base de un PIE siempre sale igual al depurar. A mí me despistó al principio.*

*Nota sobre el método: todo lo que aparece aquí ha sido previamente investigado y verificado con las herramientas pertinentes.*

Dicho esto, el binario de crackmes.one es de fdisotto y se llama good_boy: [good_boy](https://crackmes.one/crackme/669a714890c4c2830c820bc0)


### Preparando los binarios de pruebas

Antes de tocar el crackme nos conviene tener un binario del que lo sepamos todo y lo podamos comprobar. Si aplicamos un método sobre un binario stripped y nos da una dirección, no tenemos forma de saber si es la correcta, cosa que con un programa propio sí. Podemos compilarlo las dos veces que veremos (una con símbolos y otra sin ellos), aplicar el método sobre la versión sin símbolos y comprobar el resultado contra la versión que sí los tiene. 

Vamos a mostrar primero de todo el código con el que trabajaremos (aunque lo dejaré también en el directorio): 

```c
#include <stdio.h>
#include <string.h>


int main(void) {

  char buf[256];
  char clave[] = "prueba";

  printf("Introduce la clave: ");
  scanf("%255s", buf);

  if (strcmp(buf, clave) == 0) {
    printf("Correcto\n");
  } else {
    printf("Incorrecto\n");
    return 1; 
  }

  return 0;

}

```

Aquí tenemos la versión de gcc y la distro (con el comando `gcc --version`): `gcc (Debian 14.2.0-19) 14.2.0`.

Ahora debemos ejecutar estos comandos para compilar el binario optimizado: 

```bash
gcc -O2 crackme.c -o crackme_o2 
cp crackme_o2 crackme_o2_strip && strip crackme_o2_strip 
```

Ya tenemos el binario dos veces, uno con símbolos y otro sin ellos. Uso `-O2` porque es como estaba compilado el crackme de good_boy, y porque es el nivel estándar en la práctica (casi todo el software que tenemos instalado hoy en día viene compilado así. Es importante porque cambia bastante el desensamblado; guarda variables en registros en vez de en la pila, elimina el puntero de marco, y sustituye llamadas a funciones conocidas por código directo, y reordena instrucciones. Todo esto lo iremos viendo sobre el propio binario).

### Qué se lleva el strip

Dicho esto, normalmente (como hemos visto en otros writeups de este repositorio), lo primero que haríamos sería tirar el comando strings (después de haber analizado la estructura con file), pero no lo vamos a mostrar entero. Eso sí, mostraremos esto en su defecto: 

```bash
strings crackme_o2 | wc -l
83

strings crackme_o2_strip | wc -l
49
```

**¿Por qué se ha reducido la lista y qué cosas han desaparecido?** 

```bash
diff <(strings crackme_o2) <(strings crackme_o2_strip)
24,57d23
< Scrt1.o
< __abi_tag
< crackme.c
< crtstuff.c
< deregister_tm_clones
< __do_global_dtors_aux
< completed.0
< __do_global_dtors_aux_fini_array_entry
< frame_dummy
< __frame_dummy_init_array_entry
< __FRAME_END__
< _DYNAMIC
< __GNU_EH_FRAME_HDR
< _GLOBAL_OFFSET_TABLE_
< __libc_start_main@GLIBC_2.34
< _ITM_deregisterTMCloneTable
< puts@GLIBC_2.2.5
< _edata
< _fini
< printf@GLIBC_2.2.5
< __data_start
< __gmon_start__
< __dso_handle
< _IO_stdin_used
< _end
< __bss_start
< main
< __isoc99_scanf@GLIBC_2.7
< __TMC_END__
< _ITM_registerTMCloneTable
< __cxa_finalize@GLIBC_2.2.5
< _init
< .symtab
< .strtab
```

El comando diff nos enseña qué se ha llevado el strip, y conviene agrupar lo que hay ahí porque no todo es lo mismo. 

Lo primero que salta a la vista son los nombres de ficheros fuente: crackme.c, crtstuff.c, Scrt1.o. El binario llevaba escrito de qué código venía. 

Después están las funciones del runtime de C, ese código que se ejecuta antes y después de nuestro main: `frame_dummy, deregister_tm_clones, __do_global_dtors_aux, _init, _fini`. Y entre ellas, nuestro propio main. 

También desaparecen etiquetas de secciones y estructuras internas como `_GLOBAL_OFFSET_TABLE_, __bss_start, _edata, _end o _DYNAMIC`.

Los culpables son dos secciones que han desaparecido: .symtab (la tabla de símbolos) y .strtab (donde se guardan sus nombres como texto). 

Aquí hay que hacer una aclaración importante porque el comando diff puede despistar. En la lista aparecen `puts@GLIBC_2.2.5, printf@GLIBC_2.2.5 y __isoc99_scanf@GLIBC_2.7`, y parece que las llamadas a librerías se hayan perdido.

Pero no es el caso, lo que pasa es que .symtab guardaba una copia de esos nombres, y es la copia la que desaparece. Los originales viven en otras tablas distintas (.dynstr, .dynsym) que strip no toca, porque el enlazador dinámico los necesita (los nombres) en tiempo de ejecución para resolver las llamadas (cosa que posteriormente veremos en el análisis dinámico). 

Otra cosa que hay que destacar y que diff no nos dice como tal: lo que se ha borrado son los nombres, no el código. La función 'start' sigue estando en el binario y sigue ejecutándose igual, lo único que se ha perdido es la etiqueta que decía que el código de esa dirección se llamaba así. Por así decirlo, resumidamente, lo que se pierde no lo necesita el ordenador, lo necesitábamos nosotros. 

Si ejecutamos el binario, comprobamos básicamente que sigue funcionando igual:

```bash
./crackme_o2_strip 
Introduce la clave: prueba
Correcto
```

### Dos tablas de símbolos, no una

En la sección anterior habíamos dicho que los nombres de las llamadas a librería siguen en el binario aunque el diff diera a entender lo contrario. Vamos a comprobarlo, porque strings no puede resolverlo, nos devuelve una lista plana de texto sin distinguir de qué sección viene cada cadena. Para eso necesitamos una herramienta que sepa leer las tablas de símbolos por separado, y esa es `nm`.

`nm` es una herramienta que lista los símbolos de un binario, o sea los pares nombre-dirección, y según la opción lee una tabla u otra. 

```bash
nm crackme_o2
0000000000002110 r __abi_tag
0000000000004028 B __bss_start
0000000000004028 b completed.0
                 w __cxa_finalize@GLIBC_2.2.5
0000000000004018 D __data_start
0000000000004018 W data_start
0000000000001120 t deregister_tm_clones
0000000000001190 t __do_global_dtors_aux
0000000000003dd8 d __do_global_dtors_aux_fini_array_entry
0000000000004020 D __dso_handle
0000000000003de0 d _DYNAMIC
0000000000004028 D _edata
0000000000004030 B _end
00000000000011dc T _fini
00000000000011d0 t frame_dummy
0000000000003dd0 d __frame_dummy_init_array_entry
000000000000210c r __FRAME_END__
0000000000003fe8 d _GLOBAL_OFFSET_TABLE_
                 w __gmon_start__
0000000000002034 r __GNU_EH_FRAME_HDR
0000000000001000 T _init
0000000000002000 R _IO_stdin_used
                 U __isoc99_scanf@GLIBC_2.7
                 w _ITM_deregisterTMCloneTable
                 w _ITM_registerTMCloneTable
                 U __libc_start_main@GLIBC_2.34
0000000000001070 T main
                 U printf@GLIBC_2.2.5
                 U puts@GLIBC_2.2.5
0000000000001150 t register_tm_clones
00000000000010f0 T _start
0000000000004028 D __TMC_END__
```


```bash
nm crackme_o2_strip 
nm: crackme_o2_strip: no hay símbolos
```

En estos dos outputs podemos ver la diferencia de un binario a otro que contiene la .symtab y .strtab. Como en el segundo están borradas, vemos "no hay símbolos".

Ahora, con el siguiente comando, la cosa cambia. Con -D lee la tabla .dynsym (la tabla de símbolos dinámicos) con los nombres en .dynstr. Aquí viven los símbolos que el enlazador dinámico necesita en tiempo de ejecución para resolver las llamadas a librerías compartidas. Como no puede funcionar sin ellos el binario, strip no los toca: 

```bash
nm -D crackme_o2_strip 
                 w __cxa_finalize@GLIBC_2.2.5
                 w __gmon_start__
                 U __isoc99_scanf@GLIBC_2.7
                 w _ITM_deregisterTMCloneTable
                 w _ITM_registerTMCloneTable
                 U __libc_start_main@GLIBC_2.34
                 U printf@GLIBC_2.2.5
                 U puts@GLIBC_2.2.5
```

Y ahí arriba tenemos la confirmación. 


Y con los dos siguientes comandos, lo que queremos también es confirmar lo mismo a nivel de fichero. `.dynsym` y `.dynstr` están presentes en ambos binarios, y `.symtab` y `.strtab` solo en el que no está stripped:

```bash
readelf -S crackme_o2 | grep -E 'symtab|strtab|dynsym|dynstr'
  [ 5] .dynsym           DYNSYM           00000000000003d8  000003d8
  [ 6] .dynstr           STRTAB           00000000000004b0  000004b0
  [28] .symtab           SYMTAB           0000000000000000  00003048
  [29] .strtab           STRTAB           0000000000000000  000033d8
  [30] .shstrtab         STRTAB           0000000000000000  000035e1
```


```bash
readelf -S crackme_o2_strip | grep -E 'symtab|strtab|dynsym|dynstr'
  [ 5] .dynsym           DYNSYM           00000000000003d8  000003d8
  [ 6] .dynstr           STRTAB           00000000000004b0  000004b0
  [28] .shstrtab         STRTAB           0000000000000000  00003047
```

Hay algo que podemos notar al ver el output y que, ya que estamos, vamos a comentar. Vemos la sección .shstrtab, que no debemos confundir con .strtab. El prefijo sh es la clave: significa *section header*. Es la tabla de cadenas de las cabeceras de sección, no la de símbolos. Vamos a hacer un resumen:

Un ELF guarda los nombres como texto en tablas separadas, y luego cada estructura apunta a un offset dentro de la tabla que le corresponde. Hay tres:

- .strtab guarda los nombres de los símbolos main, _start, frame_dummy y la usa .symtab

- .dynstr guarda los nombres de los símbolos dinámicos (printf, puts, scanf) y .dynsym es la que la usa

- .shstrtab guarda los nombres de las secciones. La usa la tabla de cabeceras de sección. 


### Pasamos a la acción: Cómo localizar el main

El procedimiento es corto y quizá asuste más de lo que debería. Primero sacamos el entry point de la cabecera ELF, que es donde el kernel salta al arrancar el programa. Desensamblamos desde ahí y nos encontramos con `_start`, que es código del runtime de C. Dentro, buscamos una instrucción `lea` que carga una dirección en el registro `rdi`. Esa dirección es main, y se la pasa como primer argumento a `__libc_start_main`, que es quien acaba llamándolo. Con eso tenemos el offset de main dentro del binario (el desplazamiento que aparece en el lea es relativo a RIP, así que hay que calcularlo respecto a la dirección de la siguiente instrucción). 

Ese offset todavía no nos sirve para poner un breakpoint, porque el binario es PIE y en memoria estará cargado en otra dirección. Eso lo resolveremos en la siguiente sección.  



```bash
readelf -h crackme_o2_strip | grep -i entrada
  Dirección del punto de entrada:    0x10f0
```

*nota: el entry point sale de la cabecera ELF, siempre, esté el binario stripped o no.*

Veamos ahora el desensamblado que cubre esta parte: 

```bash
objdump -d -M intel --start-address=0x10f0 crackme_o2_strip | head -20

crackme_o2_strip:     formato del fichero elf64-x86-64


Desensamblado de la sección .text:

00000000000010f0 <.text+0x80>:
    10f0:	31 ed                	xor    ebp,ebp
    10f2:	49 89 d1             	mov    r9,rdx
    10f5:	5e                   	pop    rsi
    10f6:	48 89 e2             	mov    rdx,rsp
    10f9:	48 83 e4 f0          	and    rsp,0xfffffffffffffff0
    10fd:	50                   	push   rax
    10fe:	54                   	push   rsp
    10ff:	45 31 c0             	xor    r8d,r8d
    1102:	31 c9                	xor    ecx,ecx
    1104:	48 8d 3d 65 ff ff ff 	lea    rdi,[rip+0xffffffffffffff65]        # 1070 <__cxa_finalize@plt+0x10>
    110b:	ff 15 af 2e 00 00    	call   QWORD PTR [rip+0x2eaf]        # 3fc0 <__cxa_finalize@plt+0x2f60>
    1111:	f4                   	hlt
    1112:	66 2e 0f 1f 84 00 00 	cs nop WORD PTR [rax+rax*1+0x0]
```

Antes de nada, me gustaría añadir una cosa. Si estás empezando igual que yo, probablemente no hayas ido a analizar más lógica que la del main y las posibles funciones que contenga el código fuente. De ser así, me parece un buen momento para hacer una pequeña introducción para ambos. 

Este es el prólogo del `_start` (que lo vemos como .text+0x80 porque es la etiqueta que le pone objdump al no haber símbolos).

Las primeras instrucciones que vemos (`xor ebp,ebp`, `pop rsi`, `mov rdx,rsp`, `and rsp, 0xf.....f0`) son preparación del runtime. Limpia el frame pointer, recoge argc y argv de la pila, y alinea a 16 bytes como exige la ABI. Todo esto realmente es código estándar que veremos en cualquier binario. 

La instrucción clave es esta: `1104:   48 8d 3d 65 ff ff ff    lea    rdi,[rip+0xffffffffffffff65]        # 1070 <__cxa_finalize@    plt+0x10>`.

Aquí es donde `_start` carga la dirección de main en rdi para pasársela como primer argumento a `__libc_start_main`. El desplazamiento que aparece en la instrucción es un número negativo y se calcula respecto a la dirección de la siguiente instrucción (0x110b). Objdump ya lo ha calculado por nosotros y nos da el resultado en el comentario. Esa es la dirección de main.

Hay que matizar que lo de `__cxa_finalize@plt+0x10` es una etiqueta falsa. Objdump no tiene ningún símbolo para la dirección 0x1070, así que la expresa como desplazamiento desde el último símbolo que sí conoce, que resulta ser el que acabamos de mencionar. Cuando veas nombres que no encajan con el contexto, quédate con la dirección del comentario e ignora la etiqueta que viene después.  

Hacemos la comprobación con el binario que no está stripped: 

```bash
nm crackme_o2 | grep main
                 U __libc_start_main@GLIBC_2.34
0000000000001070 T main  # aquí
```
 
Efectivamente, main está en 0x1070, que es exactamente lo que habíamos deducido en el binario sin símbolos.

Pero esta es la dirección de memoria virtual estática, y en cuanto ejecutemos el programa la cosa cambia.

### PIE, ASLR y GDB

Aunque lo hemos tratado en otros writeups, considero que debemos explicar (aunque brevemente) la distinción entre PIE y ASLR. PIE es una propiedad del binario (código que funciona cargado en cualquier dirección) mientras que el ASLR es una función del kernel que aleatoriza dónde se carga. 

**¿Qué tiene que ver esto con GDB?**

Algo que yo no sabía pese a las explicaciones de los otros writeups, es que GDB desactiva el ASLR a propósito al lanzar el proceso, para que depurar sea posible. Por eso la base es siempre 0x555555554000 en x86-64 (que es la que tenemos que sumar luego a la obtenida en el análisis estático para saber la dirección real del main en un análisis dinámico). 

```text
gdb -q ./crackme_o2_strip
Reading symbols from ./crackme_o2_strip...
(No debugging symbols found in ./crackme_o2_strip)
(gdb) starti
Starting program: /home/ygm/proyectos/reversing/binary-analysis/stripped-binary/crackme_o2_strip 

Program stopped.
0x00007ffff7fe4280 in ?? ()
   from /lib64/ld-linux-x86-64.so.2
(gdb) info proc mappings
process 209762
Mapped address spaces:

Start Addr         End Addr           Size               Offset             Perms File 
0x0000555555554000 0x0000555555555000 0x1000             0x0                r--p  /home/ygm/proyectos/reversing/binary-analysis/stripped-binary/crackme_o2_strip 
0x0000555555555000 0x0000555555556000 0x1000             0x1000             r-xp  /home/ygm/proyectos/reversing/binary-analysis/stripped-binary/crackme_o2_strip 
0x0000555555556000 0x0000555555557000 0x1000             0x2000             r--p  /home/ygm/proyectos/reversing/binary-analysis/stripped-binary/crackme_o2_strip 
0x0000555555557000 0x0000555555559000 0x2000             0x2000             rw-p  /home/ygm/proyectos/reversing/binary-analysis/stripped-binary/crackme_o2_strip 
0x00007ffff7fc1000 0x00007ffff7fc5000 0x4000             0x0                r--p  [vvar] 
0x00007ffff7fc5000 0x00007ffff7fc7000 0x2000             0x0                r-xp  [vdso] 
0x00007ffff7fc7000 0x00007ffff7fc8000 0x1000             0x0                r--p  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 
0x00007ffff7fc8000 0x00007ffff7ff0000 0x28000            0x1000             r-xp  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 
0x00007ffff7ff0000 0x00007ffff7ffb000 0xb000             0x29000            r--p  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 
0x00007ffff7ffb000 0x00007ffff7ffe000 0x3000             0x34000            rw-p  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 
0x00007ffff7ffe000 0x00007ffff7fff000 0x1000             0x0                rw-p   
0x00007ffffffde000 0x00007ffffffff000 0x21000            0x0                rw-p  [stack] 
```

La podemos ver en la primera línea después de ejecutar `info proc mappings`.

Antes de explicar lo que viene, quiero añadir algo. Las 4 primeras líneas (las que contienen los cincos) son los segmentos que se mapean en memoria. No entraremos en detalle en este writeup. Pero lo que sí debemos expicar es la linea que vemos cuando ejecutamos `starti`.

`0x00007ffff7fe4280 in ?? () from /lib64/ld-linux-x86-64.so.2`: En un ejecutable dinámico, el kernel no salta al entry point, primero carga el enlazador dinámico, que mapea libc y resuelve las llamadas, y solo después salta a `_start`. Por eso starti para dentro del loader. 

El siguiente comando lógico a ejecutar, sería este:

```text
(gdb) b *0x555555554000+0x1070
Breakpoint 1 at 0x555555555070
(gdb) continue
Continuing.
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".

Breakpoint 1, 0x0000555555555070 in ?? ()
```

Habiendo puesto el breakpoint en lo que creemos que es el main (lo confirmaremos ahora con los siguientes comandos), quiero añadir otra cosa: nosotros usaremos `x/8i $rip`, pero rip solo existe en la arquitectura x86-64. Podríamos ejecutar el mismo comando que es de uso general y, además, funcionaría en ARM, MIPS o RISC-V: `x/8i $pc`. `pc` es program counter.

Vayamos a ello:

```text
Breakpoint 1, 0x0000555555555070 in ?? ()
(gdb) x/8i $rip
=> 0x555555555070:	push   rbx
   0x555555555071:	lea    rdi,[rip+0xf8c]        # 0x555555556004
   0x555555555078:	xor    eax,eax
   0x55555555507a:	sub    rsp,0x110
   0x555555555081:	mov    DWORD PTR [rsp+0x9],0x65757270
   0x555555555089:	mov    DWORD PTR [rsp+0xc],0x616265
   0x555555555091:	call   0x555555555040 <printf@plt>
   0x555555555096:	lea    rsi,[rsp+0x10]
```

Aunque no hemos mostrado el volcado de toda la sección .text, podemos confirmarlo porque en estas dos instrucciones: 

```text
0x555555555081:  mov    DWORD PTR [rsp+0x9],0x65757270                
0x555555555089:  mov    DWORD PTR [rsp+0xc],0x616265  
```

Vemos que 0x9 relativo a rsp y 0xc relativo a rsp contienen prueba con el byte nulo al final. Además, se ha solapado la 'e' del medio, parece que por optimización del compilador. Es decir, rsp+0x9 contiene 'prue' (recordemos little endian) y 0x00616265 (objdump omite los ceros a la izquierda al mostrarlo, pero vemos que son 4 bytes por el bus de datos) contiene 'eba\0'. 

'prueba\0' son 7 bytes, que no es múltiplo de 4. En vez de hacer una escritura de 4, una de 2 y una de 1, el compilador ha hecho dos de 4 solapando 1 byte. 

Aquí está la prueba: 

```text
b *0x555555555091
Breakpoint 2 at 0x555555555091
(gdb) c
Continuing.

Breakpoint 2, 0x0000555555555091 in ?? ()
(gdb) x/s $rsp+9
0x7fffffffda89:	"prueba"
```

### Aplicando la teoría en un caso real

Antes de empezar con esto, hay algo que vale la pena mencionar. Tanto en mi binario stripped con optimización -O2 como en el de good_boy, nos hemos encontrado que el main estaba situado antes que `_start`. Como ha ocurrido dos veces, creo que es importante mencionarlo. Lo que he leído es esto: con -O2, gcc coloca el main en `.text.startup` y acaba **antes** que `_start`. 

El motivo es este: cuando compilas con -O2, gcc coloca cada función en una subsección según cuántas veces espera que se ejecute. `main` corre una sola vez por proceso, así que va a `.text.startup`. Las funciones que el compilador considera calientes van a `.text.hot`, y las que cree que casi nunca se ejecutan a `.text.unlikely`. La idea es agrupar por cercanía, si el código que se ejecuta junto está junto en memoria, la caché de instrucciones rinde mejor.

Vuelvo a dejar por aquí el binario de crackmes.one: [fdisotto's good_boy](https://crackmes.one/crackme/669a714890c4c2830c820bc0)

Primero de todo, una comprobación: 

```bash
file good_boy 
good_boy: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, stripped
```

Ahora vamos a identificar el entry point: 

```bash
readelf -h good_boy | grep entrada
  Dirección del punto de entrada:    0x1170
```

Ahora, desensamblamos las primeras líneas que nos marca con objdump:

```bash
objdump -d -M intel --start-address=0x1170 good_boy | head -20

good_boy:     formato del fichero elf64-x86-64


Desensamblado de la sección .text:

0000000000001170 <.text+0xb0>:
    1170:	f3 0f 1e fa          	endbr64
    1174:	31 ed                	xor    ebp,ebp
    1176:	49 89 d1             	mov    r9,rdx
    1179:	5e                   	pop    rsi
    117a:	48 89 e2             	mov    rdx,rsp
    117d:	48 83 e4 f0          	and    rsp,0xfffffffffffffff0
    1181:	50                   	push   rax
    1182:	54                   	push   rsp
    1183:	4c 8d 05 46 01 00 00 	lea    r8,[rip+0x146]        # 12d0 <__isoc99_scanf@plt+0x220>
    118a:	48 8d 0d cf 00 00 00 	lea    rcx,[rip+0xcf]        # 1260 <__isoc99_scanf@plt+0x1b0>
    1191:	48 8d 3d 28 ff ff ff 	lea    rdi,[rip+0xffffffffffffff28]        # 10c0 <__isoc99_scanf@plt+0x10>
    1198:	ff 15 42 2e 00 00    	call   QWORD PTR [rip+0x2e42]        # 3fe0 <__isoc99_scanf@plt+0x2f30>
    119e:	f4                   	hlt
```

Ya tenemos donde empieza el main, que es 0x10c0. Basándonos en toda la explicación, aunque vemos 3 instrucciones `lea`, la que nos interesa es la que contiene el registro rdi, porque es el primer argumento de `__libc_start_main`. Los otros dos (r8 y rcx) son las funciones de inicialización y finalización. 


Antes de pasar a encontrar su offset en el análisis dinámico, aquí hay algo interesante. Como confirmación adicional de que 0x10c0 es `main`, vemos que ahí dentro hay un `lea` cargando una dirección de .rodata; es decir, una cadena del código fuente:

```bash
objdump -d -M intel --start-address=0x10c0 good_boy | head -20

good_boy:     formato del fichero elf64-x86-64


Desensamblado de la sección .text:

00000000000010c0 <.text>:
    10c0:	f3 0f 1e fa          	endbr64
    10c4:	48 81 ec 28 01 00 00 	sub    rsp,0x128
    10cb:	ba 72 00 00 00       	mov    edx,0x72
    10d0:	bf 01 00 00 00       	mov    edi,0x1
    10d5:	64 48 8b 04 25 28 00 	mov    rax,QWORD PTR fs:0x28
    10dc:	00 00 
    10de:	48 89 84 24 18 01 00 	mov    QWORD PTR [rsp+0x118],rax
    10e5:	00 
    10e6:	31 c0                	xor    eax,eax
    10e8:	48 8d 35 15 0f 00 00 	lea    rsi,[rip+0xf15]        # 2004 <__isoc99_scanf@plt+0xf54>    <-- .rodata
    10ef:	66 89 54 24 0e       	mov    WORD PTR [rsp+0xe],dx
    10f4:	c7 44 24 0a 68 34 78 	mov    DWORD PTR [rsp+0xa],0x30783468
    10fb:	30 
```

Ahora, para corroborar lo anterior, vamos a buscarlo en gdb: 

```text
gdb -q ./good_boy
Reading symbols from ./good_boy...
(No debugging symbols found in ./good_boy)
(gdb) starti
Starting program: /home/ygm/crackmes/fdisotto-goodboy/good_boy 

Program stopped.
0x00007ffff7fe4280 in ?? () from /lib64/ld-linux-x86-64.so.2  <--- linker
```

De esta parte pongo solo la primera línea para no tener un output grande:

```text
(gdb) info proc mappings
process 212512
Mapped address spaces:

Start Addr         End Addr           Size               Offset             Perms File 
0x0000555555554000 0x0000555555555000 0x1000             0x0                r--p  /home/ygm/crackmes/fdisotto-goodboy/good_boy 
```

Y ahora el resto es para confirmar que no nos hemos equivocado: 

```text
(gdb) b *0x0000555555554000+0x10c0
Breakpoint 1 at 0x5555555550c0
(gdb) c
Continuing.
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".

Breakpoint 1, 0x00005555555550c0 in ?? ()
(gdb) x/15i $rip
=> 0x5555555550c0:	endbr64
   0x5555555550c4:	sub    rsp,0x128
   0x5555555550cb:	mov    edx,0x72
   0x5555555550d0:	mov    edi,0x1
   0x5555555550d5:	mov    rax,QWORD PTR fs:0x28
   0x5555555550de:	mov    QWORD PTR [rsp+0x118],rax
   0x5555555550e6:	xor    eax,eax
   0x5555555550e8:	lea    rsi,[rip+0xf15]        # 0x555555556004    <--- .rodata
   0x5555555550ef:	mov    WORD PTR [rsp+0xe],dx
   0x5555555550f4:	mov    DWORD PTR [rsp+0xa],0x30783468
   0x5555555550fc:	call   0x5555555550a0 <__printf_chk@plt>
   0x555555555101:	lea    rsi,[rsp+0x10]
   0x555555555106:	lea    rdi,[rip+0xf0c]        # 0x555555556019
   0x55555555510d:	xor    eax,eax
   0x55555555510f:	call   0x5555555550b0 <__isoc99_scanf@plt>
(gdb) 
```

Y ahora, ya podemos ir a resolver el crackme por nuestra cuenta.


**En un próximo writeup quiero cubrir un caso distinto: un crackme donde lo que decía el desensamblado no encajaba con lo que el programa acababa pidiendo. Aquí el problema era que faltaba información; allí, que la información engañaba.**
