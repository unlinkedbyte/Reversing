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
