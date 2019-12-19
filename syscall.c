/*
Copyright (C) 2019 Bailey Defino
<https://bdefino.github.io>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* execute a system call from the command line */

enum endian {
  ENDIAN_BIG,
  ENDIAN_LITTLE
};

/* return the native endianness */
enum endian endianness(void) {
  short test;

  test = 0xFF;
  return *((char *) &test) ? ENDIAN_LITTLE : ENDIAN_BIG;
}

/* decode a hexadecimal character */
int hex_decodec(uint8_t *dest, char c) {
  if (dest == NULL) {
    return -EFAULT;
  }

  /* decode */

  c = tolower(c);

  if (c >= '0'
      && c <= '9') {
    *dest = c - '0';
  } else if (c >= 'a'
      && c <= 'f') {
    *dest = c - 'a' + 10;
  } else {
    /* not a hexadecimal character */

    return -EINVAL;
  }
  return 0;
}

/*
interpret a string as an unsigned long in hexadecimal (native endianness),
and put the result into `dest`
*/
int xtoul(unsigned long *dest, const char *a);

int main(int argc, char **argv) {
  unsigned long args[7];
  int count;
  int i;

  if (!argc) {
    printf("No argument vector (this shouldn't happen)?\n");
    return 1;
  }

  if (argc < 2) {
    printf("%s - make a system call\n" \
      "Usage: %s [ULONG | 0xHEXULONG | -A CARRAY] ...\n", *argv, *argv);
    return 1;
  }

  /* fill slots */

  count = sizeof(args) / sizeof(args[0]);

  if (argc < count)
    count = argc;

  for (i = 1; i < count; i++) {
    if (*argv[i] == '0'
        && tolower(*(argv[i] + 1)) == 'x') {
      /* seems like it's a hex argument */
      
      if (xtoul(&args[i - 1], argv[i])) {
        fprintf(stderr, "Saw \"0x\" prefix, but got malformed hex: \"%s\"\n",
          argv[i]);
        return 1;
      }
    } else if (!strncmp(argv[i], "-A", 2)) {
      /* character array */

      if (strlen(argv[i]) > 2) {
        args[i - 1] = (unsigned long) (argv[i] + 2);
      } else if (i < count - 1) {
        args[i - 1] = (unsigned long) argv[i + 1];
        i++;
      } else {
        /* no argument */

        fprintf(stderr, "\"-A\" requires an argument.\n\"");
        return 1;
      }
    } else {
      /* probably an integer argument */

      args[i - 1] = atoi(argv[i]);
    }
  }

  /* fill remaining slots */

  for (; i <= sizeof(args) / sizeof(args[0]); i++)
    args[i - 1] = 0L;

  /* confirm */

  printf("Execute `syscall(");
  
  for (i = 0; i < (sizeof(args) / sizeof(args[0])) - 1; i++)
    printf("0x%lx, ", args[i]);
  printf("0x%lx)`? [Y/n] ", args[i]);
  fflush(stdout);

  if (tolower(getchar()) != 'y')
    abort();

  /* execute the syscall */

  printf("\t->0x%lx\n.", syscall(args[0], args[1], args[2], args[3], args[4],
    args[5], args[6]));
}

int xtoul(unsigned long *dest, const char *a) {
  uint8_t c;
  int i;
  int retval;
  int shift;

  if (a == NULL)
    return -EFAULT;

  if (strlen(a) < 2
      || strlen(a) > 2 + 2 * sizeof(*dest)
      || *a != '0'
      || (a[1] != 'X'
        && a[1] != 'x'))
    return -EINVAL;

  if (dest == NULL)
    return -EFAULT;

  /* read into dest */

  *dest = 0;

  for (i = strlen(a) - 1, shift = 0; i >= 2; i--, shift += 4) {
    retval = hex_decodec(&c, a[i]);

    if (retval)
      return retval;
    *dest |= ((unsigned long) c) << shift;
  }
  return 0;
}

