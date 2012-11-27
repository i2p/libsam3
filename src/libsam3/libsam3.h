/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 * we are the Borg. */
#ifndef LIBSAM3_H
#define LIBSAM3_H

#ifdef __cplusplus
extern "C" {
#endif


////////////////////////////////////////////////////////////////////////////////
/* returns fd or -1 */
extern int tcpConnect (const char *hostname, int port);

/* <0: error; 0: ok */
extern int tcpDisconnect (int fd);

/* <0: error; 0: ok */
/* sends the whole buffer */
extern int tcpSend (int fd, const void *buf, int bufSize);

/* <0: received (-res) bytes; read error */
/* can return less that requesten bytes even if `allowPartial` is 0 when connection is closed */
extern int tcpReceiveEx (int fd, void *buf, int bufSize, int allowPartial);

extern int tcpReceive (int fd, void *buf, int bufSize);

extern int tcpPrintf (int fd, const char *fmt, ...) __attribute__((format(printf,2,3)));

extern int tcpReceiveStr (int fd, char *dest, int maxSize);


////////////////////////////////////////////////////////////////////////////////
typedef struct SAMFieldList {
  char *name;
  char *value;
  struct SAMFieldList *next;
} SAMFieldList;


extern void samFreeFieldList (SAMFieldList *list);
extern void samDumpFieldList (const SAMFieldList *list);

/* NULL: error; else: list of fields */
/* first item is always 2-word reply, with first word in name and second in value */
extern SAMFieldList *samReadReply (int fd);

/*
 * example:
 *   r0: 'HELLO'
 *   r1: 'REPLY'
 *   field: NULL or 'RESULT'
 *   VALUE: NULL or 'OK'
 * returns bool
 */
extern int samIsGoodReply (const SAMFieldList *list, const char *r0, const char *r1, const char *field, const char *value);

extern const char *samFindField (const SAMFieldList *list, const char *field);


#ifdef __cplusplus
}
#endif
#endif
