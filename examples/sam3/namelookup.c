/*
 * Copyright Â© 2023 I2P
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the âSoftwareâ), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED âAS ISâ, WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://git.idk.i2p/i2p-hackers/libsam3/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../libsam3/libsam3.h"

int main(int argc, char *argv[]) {
  Sam3Session ses;
  //
  //
  libsam3_debug = 1;
  //
  if (argc < 2) {
    printf("usage: %s name [name...]\n", argv[0]);
    return 1;
  }
  /** for each name in arguments ... */
  for (int n = 1; n < argc; ++n) {
    if (!getenv("I2P_LOOKUP_QUIET")) {
      fprintf(stdout, "%s ... ", argv[n]);
      fflush(stdout);
    }
    /** do oneshot name lookup */
    if (sam3NameLookup(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, argv[n]) >=
        0) {
      fprintf(stdout, "%s\n\n", ses.destkey);
    } else {
      fprintf(stdout, "FAILED [%s]\n", ses.error);
    }
  }
  //
  return 0;
}
