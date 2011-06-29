#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    exit(1);
  }
  FILE *in = fopen(argv[1], "r");
  FILE *out = fopen(argv[2], "w");
  if (!in || !out) {
    exit(1);
  }
  int branch;
  fscanf(in, "%d", &branch);
  switch (branch) {
    case 0:
    case 1:
      fprintf(out, "foo\n");
      break;
    case 2:
      fprintf(out, "bar\n");
      break;
    case 3:
      sleep(5);
      fprintf(out, "baz\n");
      break;
  }
}
