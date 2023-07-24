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
  char cmd[1024], destkey[617]; // 616 chars + \0
  //
  libsam3_debug = 1;
  //
  memset(destkey, 0, sizeof(destkey));
  //
  if (argc < 2) {
    FILE *fl = fopen(KEYFILE, "rb");
    //
    if (fl != NULL) {
      if (fread(destkey, 616, 1, fl) == 1) {
        fclose(fl);
        goto ok;
      }
      fclose(fl);
    }
    printf("usage: streamc PUBKEY\n");
    return 1;
  } else {
    if (!sam3CheckValidKeyLength(argv[1])) {
      fprintf(stderr, "FATAL: invalid key length! %s %lu\n", argv[1],
              strlen(argv[1]));
      return 1;
    }
    strcpy(destkey, argv[1]);
  }
  //
ok:
  printf("creating session...\n");
  // create TRANSIENT session
  if (sam3CreateSilentSession(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT,
                              SAM3_DESTINATION_TRANSIENT, SAM3_SESSION_STREAM,
                              4, NULL) < 0) {
    fprintf(stderr, "FATAL: can't create session\n");
    return 1;
  }
  //
  printf("connecting...\n");
  if ((conn = sam3StreamConnect(&ses, destkey)) == NULL) {
    fprintf(stderr, "FATAL: can't connect: %s\n", ses.error);
    sam3CloseSession(&ses);
    return 1;
  }
  //
  // now waiting for incoming connection
  printf("sending test command...\n");
  if (sam3tcpPrintf(conn->fd, "test\n") < 0)
    goto error;
  if (sam3tcpReceiveStr(conn->fd, cmd, sizeof(cmd)) < 0)
    goto error;
  printf("echo: %s\n", cmd);
  //
  printf("sending quit command...\n");
  if (sam3tcpPrintf(conn->fd, "quit\n") < 0)
    goto error;
  //
  sam3CloseConnection(conn);
  sam3CloseSession(&ses);
  return 0;
error:
  fprintf(stderr, "FATAL: some error occured!\n");
  sam3CloseConnection(conn);
  sam3CloseSession(&ses);
  return 1;
}
