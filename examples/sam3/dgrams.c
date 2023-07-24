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

#define KEYFILE "dgrams.key"

int main(int argc, char *argv[]) {
  Sam3Session ses;
  char privkey[SAM3_PRIVKEY_MAX_SIZE + 1],
       pubkey[SAM3_PUBKEY_SIZE + SAM3_CERT_SIZE + 1],
       buf[33 * 1024];

  /** quit command */
  const char *quitstr = "quit";
  const size_t quitlen = strlen(quitstr);

  /** reply response */
  const char *replystr = "reply: ";
  const size_t replylen = strlen(replystr);

  FILE *fl;
  //
  libsam3_debug = 1;
  //

  /** generate new destination keypair */
  printf("generating keys...\n");
  if (sam3GenerateKeys(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, 4) < 0) {
    fprintf(stderr, "FATAL: can't generate keys\n");
    return 1;
  }
  /** copy keypair into local buffer */
  strncpy(pubkey, ses.pubkey, sizeof(pubkey));
  strncpy(privkey, ses.privkey, sizeof(privkey));
  /** create sam session */
  printf("creating session...\n");
  if (sam3CreateSession(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, privkey,
                        SAM3_SESSION_DGRAM, 4, NULL) < 0) {
    fprintf(stderr, "FATAL: can't create session\n");
    return 1;
  }
  /** make sure we have the right destination */
  // FIXME: probably not needed
  if (strcmp(pubkey, ses.pubkey) != 0) {
    fprintf(stderr, "FATAL: destination keys don't match\n");
    sam3CloseSession(&ses);
    return 1;
  }
  /** print destination to stdout */
  printf("PUB KEY (length = %li)\n=======\n%s\n=======\n",
          strlen(ses.pubkey), ses.pubkey);
  if ((fl = fopen(KEYFILE, "wb")) != NULL) {
    /** write public key to keyfile */
    fwrite(pubkey, strlen(pubkey), 1, fl);
    fclose(fl);
  }

  /* now listen for UDP packets */
  printf("starting main loop...\n");
  for (;;) {
    /** save replylen bytes for out reply at begining */
    char *datagramBuf = buf + replylen;
    const size_t datagramMaxLen = sizeof(buf) - replylen;
    int sz, isquit;
    printf("waiting for datagram...\n");
    /** blocks until we get a UDP packet */
    sz = sam3DatagramReceive(&ses, datagramBuf, datagramMaxLen);
    if (sz < 0) {
      fprintf(stderr, "ERROR: %s\n", ses.error);
      goto error;
    }
    /** ensure null terminated string */
    datagramBuf[sz] = 0;
    /** print out datagram payload to user */
    printf("FROM\n====\n%s\n====\n", ses.destkey);
    printf("SIZE=%d\n", sz);
    printf("data: [%s]\n", datagramBuf);
    /** check for "quit" */
    isquit = (sz == quitlen && memcmp(datagramBuf, quitstr, quitlen) == 0);
    /** echo datagram back to sender with "reply: " at the beginning */
    memcpy(buf, replystr, replylen);

    if (sam3DatagramSend(&ses, ses.destkey, buf, sz + replylen) < 0) {
      fprintf(stderr, "ERROR: %s\n", ses.error);
      goto error;
    }
    /** if we got a quit command wait for 10 seconds and break out of the
     * mainloop */
    if (isquit) {
      printf("shutting down...\n");
      sleep(10); /* let dgram reach it's destination */
      break;
    }
  }
  /** close session and delete keyfile */
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 0;
error:
  /** error case, close session, delete keyfile and return exit code 1 */
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 1;
}
