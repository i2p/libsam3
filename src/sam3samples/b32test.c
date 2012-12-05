/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 * I2P-Bote: 5m77dFKGEq6~7jgtrfw56q3t~SmfwZubmGdyOLQOPoPp8MYwsZ~pfUCwud6LB1EmFxkm4C3CGlzq-hVs9WnhUV
 * we are the Borg. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../libsam3/libsam3.h"


static void testb32 (const char *src, const char *res) {
  int dlen = sam3Base32EncodedLength(strlen(src)), len;
  char dest[128];
  //
  len = sam3Base32Encode(dest, src, strlen(src));
  printf("test ");
  if (len != dlen || len != strlen(res) || strcmp(res, dest) != 0) {
    printf("FAILED!\n");
  } else {
    printf("passed\n");
  }
}


int main (int argc, char *argv[]) {
  testb32("", "");
  testb32("f", "my======");
  testb32("fo", "mzxq====");
  testb32("foo", "mzxw6===");
  testb32("foob", "mzxw6yq=");
  testb32("fooba", "mzxw6ytb");
  testb32("foobar", "mzxw6ytboi======");
  //
  return 0;
}
