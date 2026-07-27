#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error_message(void) {
  printf("Usage: <binary name> 'reversing'.\n");
  exit(EXIT_FAILURE);
}

int main (int argc,char *argv[]) {
  
  if (argc > 2 || argc < 2) { // redundancia que comento en el readme
    error_message();
  }

  if (strcmp(argv[1], "reversing") == 0) {
    printf("Acceso concedido\n");
    return 0;
  } else {
    printf("Acceso denegado\n");
    error_message();
  }

  return 0;

}
