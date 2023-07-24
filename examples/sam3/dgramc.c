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

// comment the following if you don't want to stress UDP with 'big' datagram
// seems that up to 32000 bytes can be used for localhost
// note that we need 516+6+? bytes for header; lets reserve 1024 bytes for it
#define BIG (32000 - 1024)

#define KEYFILE "dgrams.key"

int main(int argc, char *argv[]) {
  Sam3Session ses;
  char buf[1024];
  char destkey[SAM3_PUBKEY_SIZE + SAM3_CERT_SIZE + 1] = {0};
  int sz;
  size_t sizeread;
  //
  libsam3_debug = 1;
  //
  if (argc < 2) {
    FILE *fl = fopen(KEYFILE, "rb");
    //
    if (fl != NULL) {
      fprintf(stderr, "Reading key file...\n");
      sizeread = fread(destkey, sizeof(char), sizeof(destkey), fl);
      fprintf(stderr, "Read %li bytes\n", sizeread);
      if (ferror(fl) != 0) {
        fprintf(stderr, "I/O Error\n");
        return 1;
      }
      // Insure that the bytes read safely fits into destkey
      if (sizeread == sizeof(destkey)) {
        fprintf(stderr, "Error, key file is to large (> %li)\n", sizeread);
        fclose(fl);
        return 1;
      }
      fclose(fl);
      goto ok;
    }
    printf("usage: dgramc PUBKEY\n");
    return 1;
  } else {
    if (strlen(argv[1]) > (SAM3_PUBKEY_SIZE + SAM3_CERT_SIZE)) {
      fprintf(stderr, "FATAL: invalid key length (%li)!\n", strlen(argv[1]));
      return 1;
    }
    strcpy(destkey, argv[1]);
  }
  //
ok:
  printf("creating session...\n");
  /* create TRANSIENT session with temporary disposible destination */
  if (sam3CreateSession(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT,
                        SAM3_DESTINATION_TRANSIENT, SAM3_SESSION_DGRAM, 4,
                        NULL) < 0) {
    fprintf(stderr, "FATAL: can't create session\n");
    return 1;
  }
  /* send datagram */
  printf("sending test datagram...\n");
  if (sam3DatagramSend(&ses, destkey, "test", 4) < 0) {
    fprintf(stderr, "ERROR: %s\n", ses.error);
    goto error;
  }
  /** receive reply */
  if ((sz = sam3DatagramReceive(&ses, buf, sizeof(buf) - 1)) < 0) {
    fprintf(stderr, "ERROR: %s\n", ses.error);
    goto error;
  }
  /** null terminated string */
  buf[sz] = 0;
  printf("received: [%s]\n", buf);
  //
#ifdef BIG
  {
    char *big = calloc(BIG + 1024, sizeof(char));
    /** generate random string */
    sam3GenChannelName(big, BIG + 1023, BIG + 1023);
    printf("sending BIG datagram...\n");
    if (sam3DatagramSend(&ses, destkey, big, BIG) < 0) {
      free(big);
      fprintf(stderr, "ERROR: %s\n", ses.error);
      goto error;
    }
    if ((sz = sam3DatagramReceive(&ses, big, BIG + 512)) < 0) {
      free(big);
      fprintf(stderr, "ERROR: %s\n", ses.error);
      goto error;
    }
    big[sz] = 0;
    printf("received (%d): [%s]\n", sz, big);
    free(big);
  }
#endif
  //
  printf("sending quit datagram...\n");
  if (sam3DatagramSend(&ses, destkey, "quit", 4) < 0) {
    fprintf(stderr, "ERROR: %s\n", ses.error);
    goto error;
  }
  if ((sz = sam3DatagramReceive(&ses, buf, sizeof(buf) - 1)) < 0) {
    fprintf(stderr, "ERROR: %s\n", ses.error);
    goto error;
  }
  buf[sz] = 0;
  printf("received: [%s]\n", buf);
  //
  sam3CloseSession(&ses);
  return 0;
error:
  sam3CloseSession(&ses);
  return 1;
}
