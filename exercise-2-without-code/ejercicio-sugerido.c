#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(void) {
  printf("Usage: <binario> 4 digitos\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  if (argc != 2 || strlen(argv[1]) != 4) {
    error();
  }
  int suma = 0;
  for (int i = 0; i < 4; i++) {
    suma += argv[1][i] - '0';
  }
  if (suma == 20) {
    printf("Codigo correcto\n");
  } else {
    printf("Codigo incorrecto\n");
    error();
  }
  return 0;
}
