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

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "../libsam3a/libsam3a.h"

////////////////////////////////////////////////////////////////////////////////
#define KEYFILE "streams.key"

////////////////////////////////////////////////////////////////////////////////
static void ccbError(Sam3AConnection *ct) {
  fprintf(stderr,
          "\n===============================\nCONNECTION_ERROR: "
          "[%s]\n===============================\n",
          ct->error);
}

static void ccbDisconnected(Sam3AConnection *ct) {
  fprintf(stderr, "\n===============================\nCONNECTION_"
                  "DISCONNECTED\n===============================\n");
}

static void ccbConnected(Sam3AConnection *ct) {
  fprintf(stderr, "\n===============================\nCONNECTION_CONNECTED\n==="
                  "============================\n");
  // sam3aCancelConnection(ct); // cbSent() will not be called
}

static void ccbAccepted(Sam3AConnection *ct) {
  fprintf(stderr, "\n===============================\nCONNECTION_ACCEPTED\n===="
                  "===========================\n");
}

static void ccbSent(Sam3AConnection *ct) {
  fprintf(stderr, "\n===============================\nCONNECTION_WANTBYTES\n==="
                  "============================\n");
  // sam3aCancelConnection(ct);
  // sam3aCancelSession(ct->ses); // hehe
  fprintf(stderr, "(%p)\n", ct->udata);
  //
  switch ((intptr_t)ct->udata) {
  case 0:
    if (sam3aSend(ct, "test\n", -1) < 0) {
      fprintf(stderr, "SEND ERROR!\n");
      sam3aCancelSession(ct->ses); // hehe
    }
    break;
  case 1:
    if (sam3aSend(ct, "quit\n", -1) < 0) {
      fprintf(stderr, "SEND ERROR!\n");
      sam3aCancelSession(ct->ses); // hehe
    }
    break;
  default:
    return;
  }
  ct->udata = (void *)(((intptr_t)ct->udata) + 1);
}

static void ccbRead(Sam3AConnection *ct, const void *buf, int bufsize) {
  fprintf(stderr,
          "\n===============================\nCONNECTION_GOTBYTES "
          "(%d)\n===============================\n",
          bufsize);
}

static void ccbDestroy(Sam3AConnection *ct) {
  fprintf(stderr, "\n===============================\nCONNECTION_DESTROY\n====="
                  "==========================\n");
}

static const Sam3AConnectionCallbacks ccb = {
    .cbError = ccbError,
    .cbDisconnected = ccbDisconnected,
    .cbConnected = ccbConnected,
    .cbAccepted = ccbAccepted,
    .cbSent = ccbSent,
    .cbRead = ccbRead,
    .cbDestroy = ccbDestroy,
};

////////////////////////////////////////////////////////////////////////////////
static void scbError(Sam3ASession *ses) {
  fprintf(stderr,
          "\n===============================\nSESION_ERROR: "
          "[%s]\n===============================\n",
          ses->error);
}

static void scbCreated(Sam3ASession *ses) {
  char destkey[517];
  FILE *fl;
  //
  fprintf(stderr, "\n===============================\nSESION_CREATED\n");
  fprintf(stderr, "\rPRIV: %s\n", ses->privkey);
  fprintf(stderr, "\nPUB: %s\n===============================\n", ses->pubkey);
  //
  fl = fopen(KEYFILE, "rb");
  //
  if (fl == NULL) {
    fprintf(stderr, "ERROR: NO KEY FILE!\n");
    sam3aCancelSession(ses);
    return;
  }
  if (fread(destkey, 516, 1, fl) != 1) {
    fprintf(stderr, "ERROR: INVALID KEY FILE!\n");
    fclose(fl);
    sam3aCancelSession(ses);
    return;
  }
  fclose(fl);
  destkey[516] = 0;
  if (sam3aStreamConnect(ses, &ccb, destkey) == NULL) {
    fprintf(stderr, "ERROR: CAN'T CREATE CONNECTION!\n");
    sam3aCancelSession(ses);
    return;
  }
  fprintf(stderr, "GOON: creating connection...\n");
}

static void scbDisconnected(Sam3ASession *ses) {
  fprintf(stderr, "\n===============================\nSESION_DISCONNECTED\n===="
                  "===========================\n");
}

static void scbDGramRead(Sam3ASession *ses, const void *buf, int bufsize) {
  fprintf(stderr, "\n===============================\nSESION_DATAGRAM_READ\n==="
                  "============================\n");
}

static void scbDestroy(Sam3ASession *ses) {
  fprintf(stderr, "\n===============================\nSESION_DESTROYED\n======="
                  "========================\n");
}

static const Sam3ASessionCallbacks scb = {
    .cbError = scbError,
    .cbCreated = scbCreated,
    .cbDisconnected = scbDisconnected,
    .cbDatagramRead = scbDGramRead,
    .cbDestroy = scbDestroy,
};

////////////////////////////////////////////////////////////////////////////////
#define HOST SAM3A_HOST_DEFAULT
//#define HOST  "google.com"

int main(int argc, char *argv[]) {
  Sam3ASession ses;
  //
  libsam3a_debug = 0;
  //
  if (sam3aCreateSession(&ses, &scb, HOST, SAM3A_PORT_DEFAULT,
                         SAM3A_DESTINATION_TRANSIENT,
                         SAM3A_SESSION_STREAM) < 0) {
    fprintf(stderr, "FATAL: can't create main session!\n");
    return 1;
  }
  //
  while (sam3aIsActiveSession(&ses)) {
    fd_set rds, wrs;
    int res, maxfd = 0;
    struct timeval to;
    //
    FD_ZERO(&rds);
    FD_ZERO(&wrs);
    if (sam3aIsActiveSession(&ses) &&
        (maxfd = sam3aAddSessionToFDS(&ses, -1, &rds, &wrs)) < 0)
      break;
    sam3ams2timeval(&to, 1000);
    res = select(maxfd + 1, &rds, &wrs, NULL, &to);
    if (res < 0) {
      if (errno == EINTR)
        continue;
      fprintf(stderr, "FATAL: select() error!\n");
      break;
    }
    if (res == 0) {
      fprintf(stdout, ".");
      fflush(stdout);
    } else {
      if (sam3aIsActiveSession(&ses))
        sam3aProcessSessionIO(&ses, &rds, &wrs);
    }
  }
  //
  sam3aCloseSession(&ses);
  //
  return 0;
}
