/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 * we are the Borg. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libsam3/libsam3.h"


#define KEYFILE  "streams.key"


int main (int argc, char *argv[]) {
  Sam3Session ses;
  char cmd[1024], destkey[517]; // 516 chars + \0
  //
  //libsam3_debug = 1;
  //
  memset(destkey, 0, sizeof(destkey));
  //
  if (argc < 2) {
    FILE *fl = fopen(KEYFILE, "rb");
    //
    if (fl != NULL) {
      if (fread(destkey, 516, 1, fl) == 1) {
        fclose(fl);
        goto ok;
      }
      fclose(fl);
    }
    printf("usage: dgramc PUBKEY\n");
    return 1;
  } else {
    if (strlen(argv[1]) != 516) {
      fprintf(stderr, "FATAL: invalid key length!\n");
      return 1;
    }
    strcpy(destkey, argv[1]);
  }
  //
ok:
  printf("creating session...\n");
  // create TRANSIENT session
  if (samCreateSession(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, SAM3_DESTINATION_TRANSIENT, SAM3_SESSION_STREAM) < 0) {
    fprintf(stderr, "FATAL: can't create session\n");
    return 1;
  }
  //
  printf("connecting...\n");
  if (samStreamConnect(&ses, destkey) < 0) {
    fprintf(stderr, "FATAL: can't connect: %s\n", ses.error);
    samCloseSession(&ses);
    return 1;
  }
  //
  // now waiting for incoming connection
  printf("sending test command...\n");
  if (sam3tcpPrintf(ses.fd, "test\n") < 0) goto error;
  if (sam3tcpReceiveStr(ses.fd, cmd, sizeof(cmd)) < 0) goto error;
  printf("echo: %s\n", cmd);
  //
  printf("sending quit command...\n");
  if (sam3tcpPrintf(ses.fd, "quit\n") < 0) goto error;
  //
  samCloseSession(&ses);
  return 0;
error:
  fprintf(stderr, "FATAL: some error occured!\n");
  samCloseSession(&ses);
  return 1;
}
