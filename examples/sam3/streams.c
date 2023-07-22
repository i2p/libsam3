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

#define KEYFILE "streams.key"

int main(int argc, char *argv[]) {
  Sam3Session ses;
  Sam3Connection *conn;
  FILE *fl;
  //
  libsam3_debug = 1;
  //
  printf("creating session...\n");
  // create TRANSIENT session
  if (sam3CreateSession(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT,
                        SAM3_DESTINATION_TRANSIENT, SAM3_SESSION_STREAM, 4,
                        NULL) < 0) {
    fprintf(stderr, "FATAL: can't create session\n");
    return 1;
  }
  //
  printf("PUB KEY\n=======\n%s\n=======\n", ses.pubkey);
  if ((fl = fopen(KEYFILE, "wb")) != NULL) {
    fwrite(ses.pubkey, strlen(ses.pubkey), 1, fl);
    fclose(fl);
  }
  //
  printf("starting stream acceptor...\n");
  if ((conn = sam3StreamAccept(&ses)) == NULL) {
    fprintf(stderr, "FATAL: can't accept: %s\n", ses.error);
    sam3CloseSession(&ses);
    return 1;
  }
  printf("FROM\n====\n%s\n====\n", conn->destkey);
  //
  printf("starting main loop...\n");
  for (;;) {
    char cmd[256];
    //
    if (sam3tcpReceiveStr(conn->fd, cmd, sizeof(cmd)) < 0)
      goto error;
    printf("cmd: [%s]\n", cmd);
    if (strcmp(cmd, "quit") == 0)
      break;
    // echo command
    if (sam3tcpPrintf(conn->fd, "re: %s\n", cmd) < 0)
      goto error;
  }
  //
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 0;
error:
  fprintf(stderr, "FATAL: some error occured!\n");
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 1;
}
