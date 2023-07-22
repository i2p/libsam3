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
typedef struct {
  char *str;
  int strsize;
  int strused;
  int doQuit;
} ConnData;

static void cdAppendChar(ConnData *d, char ch) {
  if (d->strused + 1 >= d->strsize) {
    // fuck errors
    d->strsize = d->strused + 1024;
    d->str = realloc(d->str, d->strsize + 1);
  }
  d->str[d->strused++] = ch;
  d->str[d->strused] = 0;
}

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
  fprintf(stderr, "FROM: %s\n===============================\n", ct->destkey);
}

static void ccbSent(Sam3AConnection *ct) {
  ConnData *d = (ConnData *)ct->udata;
  //
  fprintf(stderr, "\n===============================\nCONNECTION_WANTBYTES\n==="
                  "============================\n");
  if (d->doQuit) {
    sam3aCancelSession(ct->ses); // hehe
  }
}

static void ccbRead(Sam3AConnection *ct, const void *buf, int bufsize) {
  const char *b = (const char *)buf;
  ConnData *d = (ConnData *)ct->udata;
  //
  fprintf(stderr,
          "\n===============================\nCONNECTION_GOTBYTES "
          "(%d)\n===============================\n",
          bufsize);
  while (bufsize > 0) {
    cdAppendChar(ct->udata, *b);
    if (*b == '\n') {
      fprintf(stderr, "cmd: %s", d->str);
      if (strcasecmp(d->str, "quit\n") == 0)
        d->doQuit = 1;
      if (sam3aSend(ct, d->str, -1) < 0) {
        // sam3aCancelConnection(ct); // hehe
        sam3aCancelSession(ct->ses); // hehe
        return;
      }
      d->str[0] = 0;
      d->strused = 0;
    }
    ++b;
    --bufsize;
  }
}

static void ccbDestroy(Sam3AConnection *ct) {
  fprintf(stderr, "\n===============================\nCONNECTION_DESTROY\n====="
                  "==========================\n");
  if (ct->udata != NULL) {
    ConnData *d = (ConnData *)ct->udata;
    //
    if (d->str != NULL)
      free(d->str);
    free(d);
  }
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
  FILE *fl;
  Sam3AConnection *conn;
  //
  fprintf(stderr, "\n===============================\nSESION_CREATED\n");
  fprintf(stderr, "\rPRIV: %s\n", ses->privkey);
  fprintf(stderr, "\nPUB: %s\n===============================\n", ses->pubkey);
  //
  fl = fopen(KEYFILE, "wb");
  //
  if (fl == NULL) {
    fprintf(stderr, "ERROR: CAN'T CREATE KEY FILE!\n");
    sam3aCancelSession(ses);
    return;
  }
  if (fwrite(ses->pubkey, 516, 1, fl) != 1) {
    fprintf(stderr, "ERROR: CAN'T WRITE KEY FILE!\n");
    fclose(fl);
    sam3aCancelSession(ses);
    return;
  }
  fclose(fl);
  if ((conn = sam3aStreamAccept(ses, &ccb)) == NULL) {
    fprintf(stderr, "ERROR: CAN'T CREATE CONNECTION!\n");
    sam3aCancelSession(ses);
    return;
  }
  //
  conn->udata = calloc(1, sizeof(ConnData));
  fprintf(stderr, "GOON: accepting connection...\n");
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
