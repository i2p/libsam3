#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libsam3/libsam3.h"


int main (int argc, char *argv[]) {
  int fd;
  SAMFieldList *rep = NULL;
  const char *v;
  //
  if ((fd = tcpConnect("localhost", 7656)) < 0) return 1;
  //
  if (tcpPrintf(fd, "HELLO VERSION MIN=3.0 MAX=3.0\n") < 0) goto error;
  rep = samReadReply(fd);
  samDumpFieldList(rep);
  if (!samIsGoodReply(rep, "HELLO", "REPLY", "RESULT", "OK")) goto error;
  samFreeFieldList(rep);
  rep = NULL;
  //
  if (tcpPrintf(fd, "DEST GENERATE\n") < 0) goto error;
  rep = samReadReply(fd);
  //samDumpFieldList(rep);
  if (!samIsGoodReply(rep, "DEST", "REPLY", "PUB", NULL)) goto error;
  if (!samIsGoodReply(rep, "DEST", "REPLY", "PRIV", NULL)) goto error;
  v = samFindField(rep, "PUB");
  printf("PUB KEY\n=======\n%s\n", v);
  v = samFindField(rep, "PRIV");
  printf("PRIV KEY\n========\n%s\n", v);
  samFreeFieldList(rep);
  rep = NULL;
  //
  samFreeFieldList(rep);
  tcpDisconnect(fd);
  return 0;
error:
  samFreeFieldList(rep);
  tcpDisconnect(fd);
  return 1;
}
