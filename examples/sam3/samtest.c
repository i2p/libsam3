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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../libsam3/libsam3.h"

int main(int argc, char *argv[]) {
  int fd;
  SAMFieldList *rep = NULL;
  const char *v;
  //
  libsam3_debug = 1;
  //
  //
  if ((fd = sam3Handshake(NULL, 0, NULL)) < 0)
    return 1;
  //
  if (sam3tcpPrintf(fd, "DEST GENERATE\n") < 0)
    goto error;
  rep = sam3ReadReply(fd);
  // sam3DumpFieldList(rep);
  if (!sam3IsGoodReply(rep, "DEST", "REPLY", "PUB", NULL))
    goto error;
  if (!sam3IsGoodReply(rep, "DEST", "REPLY", "PRIV", NULL))
    goto error;
  v = sam3FindField(rep, "PUB");
  printf("PUB KEY\n=======\n%s\n", v);
  v = sam3FindField(rep, "PRIV");
  printf("PRIV KEY\n========\n%s\n", v);
  sam3FreeFieldList(rep);
  rep = NULL;
  //
  sam3FreeFieldList(rep);
  sam3tcpDisconnect(fd);
  return 0;
error:
  sam3FreeFieldList(rep);
  sam3tcpDisconnect(fd);
  return 1;
}
