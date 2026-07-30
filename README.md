### Reversing

*Este repositorio, a diferencia de los demás, está en español porque documentar el razonamiento técnico me sale más natural en mi idioma nativo, ya que escribir todo esto lleva muchas horas detrás. Puede que en el futuro traduzca parte o todo el contenido*

Hace un par de semanas, el 13 de julio, antes de subir mi primer post o incluso de crear el blog, decidí empezar con la ingeniería inversa porque había invertido todo el tiempo en tratar de construir unas bases en C construyendo herramientas. Pero últimamente el k&r me quitaba mucha energía. Como tampoco debemos quemarnos innecesariamente (y esto es más una carrera de fondo), decidí crearme la VM con el liveCD del libro de jon erikson para trastear por primera vez con lo que me llamaba la atención, pudiendo leer C mucho mejor que hacía un par de meses (esto, en esencia, es lo que me permitió quitarme ese gusanillo). Así que esa necesidad de sentir un avance real y también divertirme aprendiendo es lo que impulsó esto, haciendo, a su vez, que escribiera mi primer post con una idea, en mi opinión, interesante (y relacionada con todo mi github dicho sea de paso). Aunque pueden parecer muchos frentes abiertos, creo sinceramente que tampoco es una mala combinación si sabes organizarte. Además, el hecho de ver código en C y su traducción al Assembly puede acelerar tu aprendizaje. Este writeup es mi evolución en este par de semanas. Y si yo "he podido" en este tiempo, no sientas miedo del Assembly, tú también puedes. 

Por el momento será un repositorio dedicado a hacer ingeniería inversa y para mostrar la evolución en estas dos semanas. No descarto que pueda mutar en un futuro hacia cosas mejores. 

## Análisis 

* **[binary analysis](https://github.com/unlinkedbyte/Reversing/tree/main/binary-analysis)** - análisis de binarios, empezando por ejercicios con y sin el código fuente.

* **[firmware-analysis](https://github.com/unlinkedbyte/Reversing/tree/main/firmware-analysis)** - análisis de firmware real (router TP-Link, MIPS), desde el desempaquetado con binwalk hasta el seguimiento de referencias en Ghidra.

