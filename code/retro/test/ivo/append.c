#include <stdio.h>

int main() {
  FILE *fp = fopen("blah", "a");
  fprintf(fp, "hello!\n");
  fclose(fp);
  return 0;
}
