/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 * we are the Borg. */
#include "libsam3.h"

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>


/* returns fd or -1 */
int tcpConnect (const char *hostname, int port) {
  struct hostent *host = NULL;
  struct sockaddr_in addr;
  int fd, timeout = 0;
  //
  if (hostname == NULL || !hostname[0] || port < 1 || port > 65535) return -1;
  //
  host = gethostbyname(hostname);
  if (host == NULL || host->h_name == NULL || !host->h_name[0]) {
    fprintf(stderr, "ERROR: can't resolve '%s'\n", hostname);
    return -1;
  }
  //
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "ERROR: can't create socket\n");
    return -1;
  }
  //
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr = *((struct in_addr *)host->h_addr);
  //
  //fprintf(stderr, "connecting to %s [%s:%d]...\n", hostname, inet_ntoa(addr.sin_addr), port);
  //
  if (timeout > 0) {
    struct timeval tv;
    int val = 1, size = size = sizeof(val);
    //
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, size);
    //
    tv.tv_sec = timeout; tv.tv_usec = 0; size = sizeof(tv);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, size)) fprintf(stderr, "ERROR: can't set receive timeout\n");
    //
    tv.tv_sec = timeout; tv.tv_usec = 0; size = sizeof(tv);
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, size)) fprintf(stderr, "ERROR: can't set send timeout\n");
  }
  //
  if (connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
    fprintf(stderr, "ERROR: can't connect\n");
    close(fd);
    return -1;
  }
  //
  return fd;
}


// <0: error; 0: ok
int tcpDisconnect (int fd) {
  if (fd >= 0) {
    shutdown(fd, SHUT_RDWR);
    close(fd);
    return 0;
  }
  //
  return -1;
}


// <0: error; 0: ok
int tcpSend (int fd, const void *buf, int bufSize) {
  const char *c = (const char *)buf;
  //
  if (fd < 0 || (buf == NULL && bufSize > 0)) return -1;
  //
  while (bufSize > 0) {
    int wr = send(fd, c, bufSize, MSG_NOSIGNAL);
    //
    if (wr < 0 && errno == EAGAIN) continue; // interrupted by signal
    if (wr <= 0) return -1; // either error or
    c += wr;
    bufSize -= wr;
  }
  //
  return 0;
}


/* <0: received (-res) bytes; read error */
/* can return less that requesten bytes even if `allowPartial` is 0 when connection is closed */
int tcpReceiveEx (int fd, void *buf, int bufSize, int allowPartial) {
  char *c = (char *)buf;
  int total = 0;
  //
  if (fd < 0 || (buf == NULL && bufSize > 0)) return -1;
  //
  while (bufSize > 0) {
    int rd = recv(fd, c, bufSize, 0);
    //
    if (rd < 0 && errno == EAGAIN) continue; // interrupted by signal
    if (rd == 0) return total;
    if (rd < 0) return -total;
    c += rd;
    total += rd;
    bufSize -= rd;
    if (allowPartial) break;
  }
  //
  return total;
}


int tcpReceive (int fd, void *buf, int bufSize) {
  return tcpReceiveEx(fd, buf, bufSize, 0);
}


__attribute__((format(printf,2,3))) int tcpPrintf (int fd, const char *fmt, ...) {
  int res;
  char buf[512], *p = buf;
  int size = sizeof(buf)-1;
  //
  for (;;) {
    va_list ap;
    char *np;
    int n;
    //
    va_start(ap, fmt);
    n = vsnprintf(p, size, fmt, ap);
    va_end(ap);
    //
    if (n > -1 && n < size) break;
    if (n > -1) size = n+1; else size *= 2;
    if (p == buf) {
      if ((p = malloc(size+4)) == NULL) return -1;
    } else {
      if ((np = realloc(p, size+4)) == NULL) { free(p); return -1; }
      p = np;
    }
  }
  //
  res = tcpSend(fd, p, strlen(p));
  if (p != buf) free(p);
  return res;
}


int tcpReceiveStr (int fd, char *dest, int maxSize) {
  char *d = dest;
  //
  if (maxSize < 1 || fd < 0 || dest == NULL) return -1;
  memset(dest, 0, maxSize);
  while (maxSize > 1) {
    char *e;
    int rd = recv(fd, d, maxSize-1, MSG_PEEK);
    //
    if (rd < 0 && errno == EAGAIN) continue; // interrupted by signal
    if (rd == 0) {
      rd = recv(fd, d, 1, 0);
      if (rd < 0 && errno == EAGAIN) continue; // interrupted by signal
      if (d[0] == '\n') {
        d[0] = 0; // remove '\n'
        return 0;
      }
    } else {
      if (rd < 0) return -1; // error or connection closed; alas
    }
    // check for EOL
    d[maxSize-1] = 0;
    if ((e = strchr(d, '\n')) != NULL) {
      rd = e-d+1; // bytes to receive
      if (tcpReceive(fd, d, rd) < 0) return -1; // alas
      d[rd-1] = 0; // remove '\n'
      return 0; // done
    } else {
      // let's receive this part and go on
      if (tcpReceive(fd, d, rd) < 0) return -1; // alas
      maxSize -= rd;
      d += rd;
    }
  }
  // alas, the string is too big
  return -1;
}


void samFreeFieldList (SAMFieldList *list) {
  while (list != NULL) {
    SAMFieldList *c = list;
    //
    list = list->next;
    if (c->name != NULL) free(c->name);
    if (c->value != NULL) free(c->value);
    free(c);
  }
}


void samDumpFieldList (const SAMFieldList *list) {
  for (; list != NULL; list = list->next) {
    fprintf(stderr, "%s=[%s]\n", list->name, list->value);
  }
}


static char *xstrdup (const char *s, int len) {
  if (len >= 0) {
    char *res = malloc(len+1);
    //
    if (res != NULL) {
      if (len > 0) memcpy(res, s, len);
      res[len] = 0;
    }
    //
    return res;
  }
  //
  return NULL;
}


// returns NULL if there are no more tokens
static inline char *xstrtokend (char *s) {
  while (*s && isspace(*s)) ++s;
  if (*s) while (*s && !isspace(*s)) ++s; else s = NULL;
  return s;
}


// NULL: error; else: list of fields
// first item is always 2-word reply, with first word in name and second in value
SAMFieldList *samReadReply (int fd) {
  SAMFieldList *first = NULL, *last, *c;
  char rep[2048], *p = rep, *e, *e1; // should be enough for any reply
  //
  if (tcpReceiveStr(fd, rep, sizeof(rep)) < 0) return NULL;
  //fprintf(stderr, ":[%s]\n", rep);
  // first 2 words
  while (*p && isspace(*p)) ++p;
  if ((e = xstrtokend(p)) == NULL) return NULL;
  if ((e1 = xstrtokend(e)) == NULL) return NULL;
  //
  if ((first = last = c = malloc(sizeof(SAMFieldList))) == NULL) return NULL;
  c->next = NULL;
  c->name = c->value = NULL;
  if ((c->name = xstrdup(p, e-p)) == NULL) goto error;
  while (*e && isspace(*e)) ++e;
  if ((c->value = xstrdup(e, e1-e)) == NULL) goto error;
  //
  p = e1;
  while (*p) {
    char oe;
    //
    while (*p && isspace(*p)) ++p;
    if ((e = xstrtokend(p)) == NULL) break; // no more tokens
    // for strchr
    oe = *e;
    *e = 0;
    //
    if ((c = malloc(sizeof(SAMFieldList))) == NULL) return NULL;
    c->next = NULL;
    c->name = c->value = NULL;
    last->next = c;
    last = c;
    //
    if ((e1 = strchr(p, '=')) != NULL) {
      // key=value
      if ((c->name = xstrdup(p, e1-p)) == NULL) goto error;
      if ((c->value = strdup(e1+1)) == NULL) goto error;
    } else {
      // only key (there is no such replies in SAMv3, but...
      if ((c->name = strdup(p)) == NULL) goto error;
    }
    *e = oe;
    p = e;
  }
  //
  return first;
error:
  samFreeFieldList(first);
  return NULL;
}


// example:
//   r0: 'HELLO'
//   r1: 'REPLY'
//   field: NULL or 'RESULT'
//   VALUE: NULL or 'OK'
// returns bool
int samIsGoodReply (const SAMFieldList *list, const char *r0, const char *r1, const char *field, const char *value) {
  if (list != NULL && list->name != NULL && list->value != NULL) {
    if (r0 != NULL && strcmp(r0, list->name) != 0) return 0;
    if (r1 != NULL && strcmp(r1, list->value) != 0) return 0;
    if (field != NULL) {
      for (list = list->next; list != NULL; list = list->next) {
        if (list->name == NULL || list->value == NULL) return 0; // invalid list, heh
        if (strcmp(field, list->name) == 0) {
          if (value != NULL && strcmp(value, list->value) != 0) return 0;
          return 1;
        }
      }
    }
    return 1;
  }
  return 0;
}


const char *samFindField (const SAMFieldList *list, const char *field) {
  if (list != NULL && field != NULL) {
    for (list = list->next; list != NULL; list = list->next) {
      if (list->name != NULL && strcmp(field, list->name) == 0) return list->value;
    }
  }
  return NULL;
}
