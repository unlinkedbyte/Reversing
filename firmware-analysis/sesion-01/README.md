### Primer análisis de firmware: TP-Link TL-WR841N V10 (firmware de 2015)

*Buena parte de la metodología y de la interpretación de las estructuras la construí con ayuda de un LLM y de fuentes externas; lo documento aquí como guía de referencia para mí mismo más que como trabajo propio. Las sesiones siguientes serán autónomas (o los firmware siguientes)*



Este será la primera parte de un análisis sobre el firmware de un router de 2015. Ya que hay muchas cosas que tocamos por primera vez, trataré de dejarlas explicadas con exhaustividad para cuando vuelva a leerme este análisis.

**Modelo:** TP-Link TL-WR841N, versión de hardware V10

**Firmware:** TL-WR841N_V10_150310 - publicado el 10 de marzo de 2015

*Nota: Por motivos de derechos de autor, no incluyo aquí en el mismo directorio el firmware original ni los archivos extraídos con binwalk ya que son propiedad de TP-Link. Puedes descargar la misma versión exacta desde la web oficial de soporte y reproducir todos los pasos de este documento con binwalk/sasquatch tal como se explicarán abajo*

## Estructura

Aunque tiene una estructura parecida, en este aspecto haremos más uso de Ghidra y de binwalk. `objdump` no entendía la ISA de MIPS, y es buena idea hacer un pequeño resumen ya sobre Ghidra, para el que la coja por primera vez. 


Esta sería la estructura que debemos seguir: 

`file sobre el .bin para identificar el formato del contenedor -> binwalk sin extraer nada para ver el mapa de lo que hay dentro y en qué offsets cae todo sin tocar nada todavía -> binwalk -e para la extracción (añadiré una pequeña nota en esta sección, ya que hubo un problema con la extracción y hubo que instalar una herramienta aparte) -> reconocimiento del sistema de archivos extraído (una navegación tipo Linux "ajeno" o familiar; vemos qué carpetas hay, cuáles son estándar (bin, etc, lib) y cuáles son propias del fabricante (web en este caso) -> triaje de objetivos (se explicara más a fondo el cómo) -> strings con greps concretos, no un volcado completo -> análisis estático con Ghidra sobre el objetivo elegido (Decompiler + Listing en paralelo, empezando por el main y por las cadenas más reveladoras encontradas en el paso anterior, en caso de que el binario contenga símbolos) -> seguimiento de referencias desde el hallazgo hasta el oigen (de la cadena sospechosa a la función que la usa, de la función a quién la llama, hasta poder afirmar "esto es alcanzable desde input externo" o "no, porque este valor es fijo")`


1. **file sobre el .bin**

(Este es el bin: `wr841nv10_wr841ndv10_en_3_16_9_up_boot\(150310\).bin`)

Identificamos el formato:

```bash
wr841nv10_wr841ndv10_en_3_16_9_up_boot(150310).bin: firmware 841 v10 TP-LINK Technologies ver. 1.0, version 3.16.9, 4063744 bytes or less, at 0x200 761358 bytes , at 0x100000 2883584 bytes 

```

Lo que confirmamos con esto, punto por punto (quiero añadir que esto es investigado sobre la marcha, no es a ciegas, por lo que los errores conceptuales intentaré reducirlos al mínimo, siendo los que haya probablemente de comprensión):

`firmware 841 v10 TP-LINK Technologies ver. 1.0`: File ha reconocido el formato propietario de TP-Link (tiene los magic bytes en sus bases) y confirma el modelo y la versión de hardware.

`version 3.16.9`: Esta es la versión del firmware en sí (distinta de la versión V10). Es un dato de metadata útil si en algún momento quisiéramos cruzarlo contra vulnerabilidades conocidas de esa versión exacta. 

`4063744 bytes or less`: Este número lo veremos luego en binwalk como el offset del kernel, en el primer header de TP-Link. Aqui `file` nos lo está describiendo como el tamaño máximo/límite de la primera zona del contenedor. Es un buen ejemplo de cómo dos herramientas distintas pueden describir el mismo número con matices de significado distintos según cómo interpretan el formato. No tengo certeza absoluta de si `file` interpreta este campo igual que `binwalk` o si hay un matiz distinto entre ambos, lo tratamos como el mismo dato visto desde dos ángulos, pendiente de confirmar. 

`at 0x200, 761358 bytes`: 0x200 es 512 en decimal, coincide también con lo que veremos en el output de binwalk con el offset de rootfs. Es la posición del propio header TP-Link donde está guardado ese campo (no la posición del rootfs en el archivo), mientras que 761358 es el valor que contiene ese campo.

`at 0x100000, 2883584 bytes`: 0x100000 = 1.048.576 en decimal, 1MB. Lo veremos como rootfs length y luego mostrará un tamaño distinto, no sé por qué pasa. 2883584 coincide con el bootloader offset que veremos en binwalk también. 

2. **binwalk sobre el comprimido**

Ojo que en el output hay bastante tralla. Yo recomendaría guardar el output en un archivo con un redireccionamiento y leerlo con el comando bat para poderle pasar un lenguaje de programación o intérprete de comandos como bash, a gusto personal, para que se vea más colorido y menos confuso. 

Aquí tenemos el output:

```bash
binwalk wr841nv10_wr841ndv10_en_3_16_9_up_boot\(150310\).bin 

DECIMAL       HEXADECIMAL     DESCRIPTION
--------------------------------------------------------------------------------
0             0x0             TP-Link firmware header, firmware version: 0.-15473.3, 
                              image version: "", product ID: 0x0, product version: 138477584, 
                              kernel load address: 0x0, kernel entry point: 0x80002000, 
                              kernel offset: 4063744, kernel length: 512, rootfs offset: 761358, 
                              rootfs length: 1048576, bootloader offset: 2883584, bootloader length: 0
13440         0x3480          U-Boot version string, "U-Boot 1.1.4 (Mar 10 2015 - 15:00:39)"
13488         0x34B0          CRC32 polynomial table, big endian
14800         0x39D0          uImage header, header size: 64 bytes, header CRC: 0x8E2B46CA, 
                              created: 2015-03-10 07:00:39, image size: 35711 bytes, 
                              Data Address: 0x80010000, Entry Point: 0x80010000, 
                              data CRC: 0x72C78246, OS: Linux, CPU: MIPS, 
                              image type: Firmware Image, compression type: lzma, 
                              image name: "u-boot image"
14864         0x3A10          LZMA compressed data, properties: 0x5D, 
                              dictionary size: 33554432 bytes, uncompressed size: 93256 bytes
131584        0x20200         TP-Link firmware header, firmware version: 0.0.3, image version: "", 
                              product ID: 0x0, product version: 138477584, 
                              kernel load address: 0x0, kernel entry point: 0x80002000, 
                              kernel offset: 3932160, kernel length: 512, 
                              rootfs offset: 761358, rootfs length: 1048576, 
                              bootloader offset: 2883584, bootloader length: 0
132096        0x20400         LZMA compressed data, properties: 0x5D, 
                              dictionary size: 33554432 bytes, uncompressed size: 2219160 bytes
1180160       0x120200        Squashfs filesystem, little endian, version 4.0, 
                              compression:lzma, size: 2477651 bytes, 560 inodes, 
                              blocksize: 131072 bytes, created: 2015-03-10 07:25:11
```

Decimal y hexadecimal nos indica la posición exacta en bytes, dentro del propio .bin, contados desde el princpio (byte 0). Es literalmente "cuántos bytes hay que avanzar desde el inicio del archivo para llegar aquí". Por si acaso hiciera falta, el decimal y el hexadecimal te dicen la misma posición, sólo cambia de base 10 a base 16. 

Description es la parte que binwalk ha reconocido en esa posición, comparando los bytes que hay ahí contra su base de datos de firmas conocidas (cabeceras TP-Link, U-Boot, LZMA, SquashFS...). 

**Línea 1(Offset 0)**: El primer header TP-Link, el mismo formato que ya vimos con `file`, con nuevos campos: 
    
    · `firmware version: 0.-15473.3`: Un número negativo raro. Después de investigarlo brevemente, no he encontrado mucho. Probablemente es cómo interpreta binwalk ese campo del header que no está pensado para mostrarte como versión legible. 

    · `Kernel load address: 0x0`: Aunque no hay certeza del motivo, parece un campo no usado por esta variante del header.

    · `Kernel entry point: 0x80002000`: La dirección real de arranque del kernel, en la zona `kseg0` de MIPS.

    · `kernel offseet: 4063744 / kernel length: 512`: Offset dentro del archivo, y una longitud que es inusual a simple vista para un kernel real (pequeña).

    · `rootfs offset: 761358 / rootfs length: 1.048.576`: mismos valores que coinciden en el file,y un desajuste detectado con el SquashFS de 2.4MB más adelante.

    · `bootloader offset: 2883584 / bootloader length: 0`: longitud cero, parece un patrón, lo marcaríamos como no fiable.

**Línea 2(offset 13440)**: `U-boot 1.1.4 (Mar 10 2015 - 15:00:39)`, el string de versión de U-Boot, con la fecha de compilación real que coincidia con la fecha de publicación del firmware.

**Línea 3(offset 13488)**: La tabla de constantes CRC32, usada por el propio U-Boot para verificar la integridad al arrancar. 

**Línea 4(offset 14800)**: El header uImage. Es el formato oficial que usa el propio U-Boot para empaquetar imágenes que va a cargar. En esta línea aparecen las direcciones "verdaderas" o, almenos, consistentes: `Data Address: 0x80010000 y Entry Point: 0x80010000` son coincidentes entre sí, algo que no pasaba en el header TP-Link de la línea 1 (donde load address era 0x0, distinto del entry point). `image name: u-boot image` confirma que este bloque concreto es el propio U-Boot comprimido, no el kernel.

**Línea 5(offset 14864)**: Los datos LZMA (el algoritmo que usa 7z frente a gzip. Es más lento pero comprime más) comprimidos correspondientes a ese U-Boot. 93256 bytes una vez descomprimidos.

**Línea 6(offset 131584)**: Un segundo header TP-Link. Parece que envuelve al kernel de linux en sí, no a U-Boot. Tiene los mismos campos que eran sospechosos repetidos, lo que parece indicar un patrón de fallo de lectura sobre esa cabecera.

**Línea 7(offset 132096)**: Los datos LZMA del kernel, 2.219.160 bytes descomprimidos. Más coherente en tamaño. 

**Línea 8(offset 1180160)**: El SquashFS. Little endian, nos muestra los inodos usados, los tamaños del bloque de 128KB, y el tamaño (2.477.651 bytes) que no coincide con el "rootfs length" de 1.048.576 declarado en los headers TP-Link de antes.

Podemos observar que los dos headers TP-Link (líneas 1 y 6) comparten exactamente los mismos valores sospechosos (kernel load address: 0x0, bootloader length: 0, rootfs length: 1048576 sin coincidir con el SquashFS real) eso no es casualidad, es evidencia de que este formato de cabecera propietario, tal como lo interpreta binwalk, tiene varios campos que no reflejan la realidad del contenido, mientras que el header uImage estándar (línea 4) sí es consistente.


**Un par de notas antes de seguir:**

kseg0 significa kernel segment + número, distingue las distintas zonas dentro de ese espacio reservado al kernel. En MIPS de 32 bits (con lo que estamos trabajando hoy), el espacio de direcciones se divide en zonas según los primeros bits de la dirección. Cualquier dirección que empiece por 0x8 cae aquí, en kseg0, y nos indica que es memoria reservada para el kernel, con caché activada, mapeada de forma fija y directa a memoria física. 

SquashFS: es un sistema de archivos de solo lectura, comprimido, pensado para dispositivos con poca memoria flash. 

La memoria flash es un tipo de chip de memoria no volátil (conserva los datos guardados aunque se apague la corriente, a diferencia de la RAM). Físicamente es un circuito integrado pequeño, soldado en la placa del dispositivo (router en este caso), sin ninguna parte móvil (nada que ver con un disco duro mecánico). Es reescribible eléctricamente cuantas veces haga falta, eso es lo que ocurre cuando actualizas el firmware de un router. Aqui es donde vive el firmware completo (bootloader + kernel + SquashFS) mientras el dispositivo esta apagado. Cuando enciendes el router, la CPU empieza a leer instrucciones desde una dirección fija que apunta a este chip flash, y de ahí empieza todo el proceso.


3. **binwalk -e para extraer**

Antes de pasar de verdad a todo el asunto, que ya es donde se pone emocionante, debemos hablar de sasquatch. Es una herramienta que nos hace falta porque el SquashFS de este firmware está "tocado" por el fabricante de una forma que el extractor estándar de binwalk no sabe desempaquetar. Nos puede llegar a hacer falta de cara al futuro si no me equivoco con ciertos firmwares, así que es mejor tenerlo instalado. 

Aquí tienes el proceso de instalación (dependencias + clonado + compilación).

```bash
sudo apt-get install zlib1g-dev liblzma-dev liblzo2-dev
git clone https://github.com/devttys0/sasquatch
cd sasquatch && ./build.sh
```

Si la compilación falla por error de -Werror o similar, aquí tienes el primer intento de arreglo: 

```bash
wget https://raw.githubusercontent.com/devttys0/sasquatch/82da12efe97a37ddcd33dba53933bc96db4d7c69/patches/patch0.txt
mv patch0.txt patches
./build.sh
```

Si no te ha quedado claro por el log de si ha funcionado o no:

```bash
which sasquatch
sasquatch --help
```

Si te sale la ruta y el panel de ayuda, los tienes. 


Ahora sí, empezamos con el proceso de extracción. Debería aparecerte algo como este output:

```bash

DECIMAL       HEXADECIMAL     DESCRIPTION
--------------------------------------------------------------------------------
14864         0x3A10          LZMA compressed data, properties: 0x5D, 
                              dictionary size: 33554432 bytes, uncompressed size: 93256 bytes
132096        0x20400         LZMA compressed data, properties: 0x5D, 
                              dictionary size: 33554432 bytes, uncompressed size: 2219160 bytes

WARNING: Symlink points outside of the extraction directory: /home/ygm/proyectos/reversing/firmware-analysis/_wr841nv10_wr841ndv10_en_3_16_9_up_boot(150310).bin.extracted/squashfs-root-0/bin/iptables-xml -> /workspace/jenkins/workspace/honeyBee_2.0_soho5_qca_trunk_prep/rootfs.build.2.6.31/sbin/iptables-multi; changing link target to /dev/null for security purposes.

WARNING: Symlink points outside of the extraction directory: /home/ygm/proyectos/reversing/firmware-analysis/_wr841nv10_wr841ndv10_en_3_16_9_up_boot(150310).bin.extracted/squashfs-root/bin/iptables-xml -> /workspace/jenkins/workspace/honeyBee_2.0_soho5_qca_trunk_prep/rootfs.build.2.6.31/sbin/iptables-multi; changing link target to /dev/null for security purposes.
1180160       0x120200        Squashfs filesystem, little endian, version 4.0, compression:lzma, size: 2477651 bytes, 560 inodes, blocksize: 131072 bytes, created: 2015-03-10 07:25:11

WARNING: One or more files failed to extract: either no utility was found or it's unimplemented
```

Antes de ver lo que haya extraído, vemos dos avisos:

Para el primer warning, nos avisa de un symlink neutralizado (iptables-xml apuntando fuera del directorio de extracción, redirigido a /dev/null). Es una mitigación de seguridad funcionando en vivo, defensa contra ataques path traversal via symlinks maliciosos.

Para el segundo warning no lo he confirmado con certeza, pero por el resto del output (SquashFS se extrajo con normalidad) parece algo puntual, no filesystem completo. Confirmamos ahora lo extraído con ls -l:


```bash
ls -l

drwxrwxr-x ygm ygm 134 B  Thu Jul 30 13:53:54 2026  _wr841nv10_wr841ndv10_en_3_16_9_up_boot(150310).bin.extracted

.rw-rw-r-- ygm ygm 3.9 MB Wed Mar 11 09:10:22 2015  wr841nv10_wr841ndv10_en_3_16_9_up_boot(150310).bin
```

Y este es el output de lo que contiene ese directorio:

```bash
ls -l _wr841nv10_wr841ndv10_en_3_16_9_up_boot\(150310\).bin.extracted/

.rw-rw-r-- ygm ygm 2.4 MB Thu Jul 30 13:53:54 2026  120200.squashfs
.rw-rw-r-- ygm ygm 2.1 MB Thu Jul 30 13:53:54 2026  20400
.rw-rw-r-- ygm ygm 3.7 MB Thu Jul 30 13:53:54 2026  20400.7z
.rw-rw-r-- ygm ygm  91 KB Thu Jul 30 13:53:54 2026  3A10
.rw-rw-r-- ygm ygm 3.9 MB Thu Jul 30 13:53:54 2026  3A10.7z
drwxr-xr-x ygm ygm  98 B  Tue Mar 10 08:25:10 2015  squashfs-root
drwxr-xr-x ygm ygm  98 B  Tue Mar 10 08:25:10 2015  squashfs-root-0
```

Esta es la confirmación de lo que habíamos visto con el primer binwalk sin extraer, ahora convertido en archivos reales. Aquí tenemos lo que podemos ver:

    · 3A10 / 3A10.7z: 0x3A10 en decimal es 14864, coincide con el offset que ya vimos como los datos LZMA de U-Boot. El .7z es la versión aún comprimida; el archivo sin extensión es el resultado ya descomprimido (93.256 bytes, como declaraba el binwalk).

    · 20400 / 20400.7z: mismo patrón, pero correspondiente al offset 0x20400 (132096), los datos LZMA del kernel Linux.

    · 120200.squashfs: el filesystem SquashFS completo, sin descomprimir su contenido interno (offset 0x120200 = 1180160, coincide exacto).

    · squashfs-root: el resultado de que sasquatch haya conseguido desempaquetar ese SquashFS en un árbol de archivos navegable. Esta es la carpeta que de verdad nos importa para lo que sigue.


Como podemos observar, los offsets y tamaños que binwalk leyó coinciden.

Vemos también que hay dos directorios squashfs-root y squashfs-root-0. Al comprobar el contenido de manera rápida, parecen idénticos. Esto es así porque ante la duda de si es little endian o big endian, sasquatch los extrae con las dos configuraciones por seguridad en carpetas separadas. Trabajaremos nosotros con squashfs-root.


4. **Reconocimiento del sistema de archivos**

```bash
$ls -la squashfs-root/
drwxr-xr-x ygm ygm  98 B  Tue Mar 10 08:25:10 2015  .
drwxrwxr-x ygm ygm 134 B  Thu Jul 30 13:53:54 2026  ..
drwxr-xr-x ygm ygm 192 B  Thu Jul 30 13:53:55 2026  bin
drwxr-xr-x ygm ygm   6 B  Tue Mar 10 08:25:10 2015  dev
drwxr-xr-x ygm ygm 244 B  Tue Mar 10 08:25:09 2015  etc
drwxr-xr-x ygm ygm 1.2 KB Tue Mar 10 08:25:09 2015  lib
lrwxrwxrwx ygm ygm  11 B  Tue Mar 10 08:24:08 2015  linuxrc ⇒ bin/busybox
drwxr-xr-x ygm ygm   0 B  Tue Mar 10 08:24:03 2015  mnt
drwxr-xr-x ygm ygm   0 B  Tue Mar 10 08:24:08 2015  proc
drwxr-xr-x ygm ygm   0 B  Tue Mar 10 08:24:03 2015 󰉐 root
drwxr-xr-x ygm ygm 498 B  Tue Mar 10 08:24:14 2015  sbin
drwxr-xr-x ygm ygm   0 B  Tue Mar 10 08:24:08 2015  sys
drwxrwxrwt ygm ygm   0 B  Tue Mar 10 08:24:04 2015  tmp
drwxr-xr-x ygm ygm  38 B  Tue Mar 10 08:24:04 2015  usr
drwxr-xr-x ygm ygm   6 B  Tue Mar 10 08:24:08 2015  var
drwxr-xr-x ygm ygm 100 B  Tue Mar 10 08:25:07 2015  web
```

Antes de entrar en cada carpeta, debemos tener un criterio de priorización, no se audita todo por igual. Como es un router, la interfaz `web` es sospechosa por defecto, procesa input directo del usuario. `/etc/` es el segundo objetivo a mirar, ya que contiene las configuraciones y, con cierta frecuencia, credenciales mal guardadas (aunque no aseguro esto en los routers modernos, debo añadir). El resto (bin/, sbin/, usr/) se recorre más por descarte o exhaustividad, mejor dicho. Miramos lo que hay y priorizamos lo que destaque por tamaño o nombre. Ojo al listar el contenido de bin, para muchos será obvio, pero recordar pasarle paralelamente el comando less. 

Un apunte importante a tener en cuenta, que esto es información nueva: BusyBox es un único binario que implementa dentro de sí mismo decenas de comandos Unix (ls, cat, mount, ps...). En vez de tener cien binarios pequeños, tienes uno grande, y todos esos comandos son, en realidad, enlaces simbólicos que apuntan a él. Cuando lo ejecutas, BusyBox mira con qué nombre fué invocado (el propio argv[0]) y decide internamente qué función ejecutar. Esto se llama multi-call binary, y es prácticamente universal en Linux embebido por ahorro de espacio flash.

Sobre proc, sys y dev no nos hará falta mirarlos. Estas tres carpetas, en un Linux real en ejecución, no contienen archivos de verdad (son pseudo sistemas de archivos, generados dinámicamente por el propio kernel para exponer información del sistema; como procesos activos, hardware, parámetros del kernel...) como si fueran archivos, aunque no ocupen espacio real en disco. Dentro de un firmware extraído como este, están vacías o casi vacías. Solo contendrían contenido real si emuláramos el firmware corriendo de verdad, como en una VM.


Dicho esto, el reconocimiento y posterior análisis de hoy se centrará en web/ (y no es poco). Dejaremos para alguna continuación análisis en profunidad del firmware, recorriendo otras carpetas.

```bash
$ls -l web/
drwxr-xr-x ygm ygm  98 B  Tue Mar 10 08:25:07 2015  dynaform
drwxr-xr-x ygm ygm  14 B  Tue Mar 10 08:25:07 2015  frames
drwxr-xr-x ygm ygm 3.5 KB Tue Mar 10 08:25:07 2015  help
drwxr-xr-x ygm ygm 286 B  Tue Mar 10 08:25:07 2015  images
drwxr-xr-x ygm ygm  64 B  Tue Mar 10 08:25:07 2015  localiztion
drwxr-xr-x ygm ygm 272 B  Tue Mar 10 08:25:07 2015  login
drwxr-xr-x ygm ygm  20 B  Tue Mar 10 08:25:08 2015  oem
drwxr-xr-x ygm ygm 3.7 KB Tue Mar 10 08:25:07 2015  userRpm
```

Dicho esto, os acordáis de lo del tamaño? aunque no supiéramos qué hace cada uno, aquí tendríamos 4 candidatos: help, images, login y userRpm. A primera vista, descartamos help e images (pero los tenemos pendiente en caso de no encontrar nada, por exhaustividad). Así que, por tamaño y/o nombre/función, nos quedamos con login/ (por motivos obvios) y userRpm.

**Qué es userRpm?** Es la carpeta donde residen las páginas .htm dinámicas de administración del router (login, cambios de contraseña, configuración de red...). Parece ser la convención de TP-Link para sus plantillas dinámicas el nombre del directorio. 

Antes de decidirnos por alguna de las dos, le eché un vistazo a ambas para ver cuál contenía menos ruido y dejarla ya cubierta. Vamos a por el de login: 

```bash
.rw-r--r-- ygm ygm 6.6 KB Tue Mar 10 08:25:07 2015 encrypt.js
.rw-r--r-- ygm ygm 4.0 KB Tue Mar 10 08:25:07 2015 loginbg.png
.rw-r--r-- ygm ygm 3.9 KB Tue Mar 10 08:25:07 2015 loginBtn.png
.rw-r--r-- ygm ygm 4.0 KB Tue Mar 10 08:25:07 2015 loginBtnH.png
.rw-r--r-- ygm ygm 3.4 KB Tue Mar 10 08:25:07 2015 loginPwd.png
.rw-r--r-- ygm ygm 3.7 KB Tue Mar 10 08:25:07 2015 loginPwdH.png
.rw-r--r-- ygm ygm 3.4 KB Tue Mar 10 08:25:07 2015 loginUser.png
.rw-r--r-- ygm ygm 3.6 KB Tue Mar 10 08:25:07 2015 loginUserH.png
.rwxr-xr-x ygm ygm 5.2 KB Tue Mar 10 08:25:07 2015 top1_1.jpg
.rwxr-xr-x ygm ygm  16 KB Tue Mar 10 08:25:07 2015 top1_2.jpg
.rwxr-xr-x ygm ygm 893 B  Tue Mar 10 08:25:07 2015 top2.jpg
.rw-r--r-- ygm ygm 605 B  Tue Mar 10 08:25:07 2015 top_bg.jpg

```

Como podemos ver, todo son imágenes a excepción de un único archivo .js . Después de analizarlo detalladamente (y debo decir que con ayuda), lo que contiene este archivo es implementación estándar de MD5 copiada de una librería pública, por lo que solo pondremos lo relevante: 

```text
function hex_md5(s)
{ 
	return binl2hex(core_md5(str2binl(s), s.length * 8));
}

```

```text

function core_md5(x, len)
{
  /* append padding */
  x[len >> 5] |= 0x80 << ((len) % 32);
  x[(((len + 64) >>> 9) << 4) + 14] = len;

  var a =  1732584193;
  var b = -271733879;
  var c = -1732584194;
  var d =  271733878;
```

En el segundo ejemplo podemos ver que estos son los vectores de inicialización oficiales del algoritmo MD5 (0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 que define el estándar, expresados aquí como enteros con signo). Cualquier implementación de MD5 real, en cualquier lenguaje, empieza exactamente con estos cuatro valores. 

También en el propio archivo hay un par de funciones (Base64Encoding y utf8_encode) que son funciones de codificación estándar, probablemente para transformar el resultado del hash (o la contraseña) antes de enviarlo.

Volviendo al primer ejemplo, podemos ver que en hex_md5 (el nombre de la función) coge la cadena s (la contraseña), la convierte a formato binario interno (str2binl), le aplica el algoritmo MD5 real (core_md5) y convierte el resultado a la representación hexadecimal legible (binl2hex). 

**Por qué esta función nos interesa?**

Si el formulario de login usa hex_md5(contraseña) en el navegador antes de mandarla al servidor, querría decir que el cliente no manda la contraseña en texto plano por http, la manda ya hasheada. MD5 está roto desde 2004-2005. Antes de especular, debemos ver el código que llama a hex_md5() y qué le pasa exactamente como argumento. En el mismo directorio no podemos ver quién llama a la función, así que se puede asumir que encrypt.js solo define la función, no la usa. Quien la llama probablemente esté en el .htm que carga este script (el propio formulario de login, que debe estar en web/). Para comprobar esto rápidamente, podemos usar este comando desde el propio directorio web o pasándole la ruta absoluta:

```bash
grep -r "hex_md5"
```

Este comando nos devuelve esto: 

```bash
login/encrypt.js:function hex_md5(s)
userRpm/ChangeLoginPwdRpm.htm:		document.forms[0].newpassword.value = Base64Encoding(hex_md5(strNewPwd));
userRpm/ChangeLoginPwdRpm.htm:		document.forms[0].newpassword2.value = Base64Encoding(hex_md5(strNewPwd2));
userRpm/ChangeLoginPwdRpm.htm:		document.forms[0].oldpassword.value = Base64Encoding(hex_md5(strOldPwd));
userRpm/LoginRpm.htm:				password = hex_md5($("pcPassword").value);	
```

`LoginRpm.htm: password = hex_md5($("pcPassword").value);` plantea un escenario malo: la contraseña se pasa directa a hex_md5(), sin ningún challenge aleatorio del servidor combinado. No hay ningún nonce, token, ni valor variable mezclado con la contraseña antes de hashear. Eso significa que el hash resultante es estático: la misma contraseña produce el mismo hash, en cada login, siempre. 

Si un atacante interceptara el tráfico una sola vez (red wifi compartida, ataque MITM, o simplemente mirando el tráfico http sin cifrar ya que este firmware habla http plano en varios sitios), captura ese hash MD5. Como no cambia nunca, podría reenviarlo directamente en futuros intentos de login sin necesidad de conocer la contraseña real. No haría falta romper el MD5 ni hacer fuerza bruta, bastaría con repetir el valor capturado. Es un ataque clásico de tipo replay (en la práctica, equivalente a un pass-the-hash): el hash actúa como la propia contraseña. 

Para ChangeLoginPwdRpm.htm vemos el mismo patrón pero en el flujo del cambio de contraseña, con el añadido de Base64Encoding() por encima. No aporta una protección real, solo le añade una capa de codificación de transporte, fácilmente reversible siendo base64. 

Como no sabemos si esto es explotable realmente (pese a que parece haber una vulnerabilidad del lado del cliente en cómo se genera y envía la contraseña), lo más correcto sería verificar el lado del servidor de esta comprobación para saberlo con certeza. Pero queda el hilo abierto, aunque el diseño por sí solo es una debilidad real. 


## Conclusiones y próximos pasos

Aunque todavía no hemos abierto Ghidra ni analizado binarios, hoy nos vamos a despedir con el análisis (o una primera pasada más bien) de web/. Hemos analizado login/ y hemos encontrado el hex_md5 al que le hemos tratado de seguir el hilo hasta loginRpm.htm y ChangeLoginPwdRpm.htm . 

Para las demás actualizaciones que iremos subiendo sobre este firmware nos faltará todavía profundizar en userRpm, etc/, sbin/ y alomejor el resto de web. 

También nos hará falta comprobar si el servidor valida ese hash sin ninguna capa adicional. 
