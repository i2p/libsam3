/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 * I2P-Bote: 5m77dFKGEq6~7jgtrfw56q3t~SmfwZubmGdyOLQOPoPp8MYwsZ~pfUCwud6LB1EmFxkm4C3CGlzq-hVs9WnhUV
 * we are the Borg. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../libsam3/libsam3.h"


#define KEYFILE  "dgrams.key"


int main (int argc, char *argv[]) {
  Sam3Session ses;
  char privkey[1024], pubkey[1024], *buf = NULL;
  FILE *fl;
  //
  //libsam3_debug = 1;
  //
  printf("generating keys...\n");
  if (sam3GenerateKeys(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT) < 0) {
    fprintf(stderr, "FATAL: can't generate keys\n");
    return 1;
  }
  //
  strcpy(pubkey, ses.pubkey);
  strcpy(privkey, ses.privkey);
  //
  printf("creating session...\n");
  if (sam3CreateSession(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, privkey, SAM3_SESSION_DGRAM, NULL) < 0) {
    fprintf(stderr, "FATAL: can't create session\n");
    return 1;
  }
  //
  if (strcmp(pubkey, ses.pubkey) != 0) {
    fprintf(stderr, "FATAL: internal error\n");
    sam3CloseSession(&ses);
    return 1;
  }
  //
  printf("PUB KEY\n=======\n%s\n=======\n", ses.pubkey);
  if ((fl = fopen(KEYFILE, "wb")) != NULL) {
    fwrite(pubkey, strlen(pubkey), 1, fl);
    fclose(fl);
  }
  //
  buf = malloc(33*1024);
  // now listen for UDP packets
  printf("starting main loop...\n");
  for (;;) {
    int sz, isquit;
    //
    printf("waiting for datagram...\n");
    if ((sz = sam3DatagramReceive(&ses, buf+4, 33*1024-4)) < 0) {
      fprintf(stderr, "ERROR: %s\n", ses.error);
      goto error;
    }
    buf[sz+4] = 0;
    //
    printf("FROM\n====\n%s\n====\n", ses.destkey);
    printf("SIZE=%d\n", sz);
    printf("data: [%s]\n", buf+4);
    isquit = (sz == 4 && memcmp(buf+4, "quit", 4) == 0);
    // echo datagram
    memcpy(buf, "re: ", 4);
    if (sam3DatagramSend(&ses, ses.destkey, buf, sz+4) < 0) {
      fprintf(stderr, "ERROR: %s\n", ses.error);
      goto error;
    }
    //
    if (isquit) {
      printf("shutting down...\n");
      sleep(10); // let dgram reach it's destination
      break;
    }
  }
  //
  if (buf != NULL) free(buf);
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 0;
error:
  if (buf != NULL) free(buf);
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 1;
}
