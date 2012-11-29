/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 * I2P-Bote: 5m77dFKGEq6~7jgtrfw56q3t~SmfwZubmGdyOLQOPoPp8MYwsZ~pfUCwud6LB1EmFxkm4C3CGlzq-hVs9WnhUV
 * we are the Borg. */
#ifndef LIBSAM3_H
#define LIBSAM3_H

#ifdef __cplusplus
extern "C" {
#endif


////////////////////////////////////////////////////////////////////////////////
extern int libsam3_debug;


////////////////////////////////////////////////////////////////////////////////
#define SAM3_HOST_DEFAULT  (NULL)
#define SAM3_PORT_DEFAULT  (0)

#define SAM3_DESTINATION_TRANSIENT  (NULL)

#define SAM3_PUBKEY_SIZE   (516)
#define SAM3_PRIVKEY_SIZE  (884)


////////////////////////////////////////////////////////////////////////////////
/* returns fd or -1 */
extern int sam3tcpConnect (const char *hostname, int port);

/* <0: error; 0: ok */
extern int sam3tcpDisconnect (int fd);

/* <0: error; 0: ok */
extern int sam3tcpSetTimeoutSend (int fd, int timeoutms);

/* <0: error; 0: ok */
extern int sam3tcpSetTimeoutReceive (int fd, int timeoutms);

/* <0: error; 0: ok */
/* sends the whole buffer */
extern int sam3tcpSend (int fd, const void *buf, int bufSize);

/* <0: received (-res) bytes; read error */
/* can return less that requesten bytes even if `allowPartial` is 0 when connection is closed */
extern int sam3tcpReceiveEx (int fd, void *buf, int bufSize, int allowPartial);

extern int sam3tcpReceive (int fd, void *buf, int bufSize);

extern int sam3tcpPrintf (int fd, const char *fmt, ...) __attribute__((format(printf,2,3)));

extern int sam3tcpReceiveStr (int fd, char *dest, int maxSize);

/* pass NULL for 'localhost' and 0 for 7655 */
extern int sam3udpSendTo (const char *hostname, int port, const void *buf, int bufSize);


////////////////////////////////////////////////////////////////////////////////
typedef struct SAMFieldList {
  char *name;
  char *value;
  struct SAMFieldList *next;
} SAMFieldList;


extern void sam3FreeFieldList (SAMFieldList *list);
extern void sam3DumpFieldList (const SAMFieldList *list);

/* read and parse SAM reply */
/* NULL: error; else: list of fields */
/* first item is always 2-word reply, with first word in name and second in value */
extern SAMFieldList *sam3ReadReply (int fd);

extern SAMFieldList *sam3ParseReply (const char *rep);

/*
 * example:
 *   r0: 'HELLO'
 *   r1: 'REPLY'
 *   field: NULL or 'RESULT'
 *   VALUE: NULL or 'OK'
 * returns bool
 */
extern int sam3IsGoodReply (const SAMFieldList *list, const char *r0, const char *r1, const char *field, const char *value);

extern const char *sam3FindField (const SAMFieldList *list, const char *field);


////////////////////////////////////////////////////////////////////////////////
/* pass NULL for 'localhost' and 0 for 7656 */
/* returns <0 on error or socket fd on success */
extern int samHandshake (const char *hostname, int port);


typedef enum {
  SAM3_SESSION_RAW,
  SAM3_SESSION_DGRAM,
  SAM3_SESSION_STREAM
} SamSessionType;


// note that for SAM3_SESSION_STREAM, fd is the 'secondary' socket used for accepting connections and data i/o
// and the 'primary' one is moved to fdd
// but for other session types, only fd is used and fdd is -1
typedef struct {
  SamSessionType type;
  int fd;
  int fdd;
  char privkey[900];
  char pubkey[520];
  char channel[66];
  char destkey[520]; // for STREAM sessions
  char error[32];
} Sam3Session;


/*
 * create SAM session
 * pass NULL as hostname for 'localhost' and 0 as port for 7656
 * pass NULL as privkey to create TRANSIENT session
 * 'params' can be NULL
 * if result<0: error, 'ses' fields are undefined
 * if result==0: ok, all 'ses' fields are filled
 */
extern int samCreateSession (Sam3Session *ses, const char *hostname, int port, const char *privkey, SamSessionType type,
  const char *params);

/* returns <0 on error, 0 on ok */
/* 'ses' must be properly initialized */
extern int samCloseSession (Sam3Session *ses);

/*
 * open stream connection to 'destkey' endpoint
 * 'destkey' is 516-byte public key
 * returns <0 on error, 0 on ok
 * you still have to call samCloseSession() on failure
 * sets ses->error on error
 */
extern int samStreamConnect (Sam3Session *ses, const char *destkey);

/*
 * accepts stream connection and sets 'destkey'
 * 'destkey' is 516-byte public key
 * returns <0 on error, 0 on ok
 * you still have to call samCloseSession() on failure
 * sets ses->error on error
 * note that there is no timeouts for now, but you can use sam3tcpSetTimeout*()
 */
extern int samStreamAccept (Sam3Session *ses);


/*
 * generate new keypair
 * fills 'privkey' and 'pubkey' only
 * you should not call samCloseSession() on 'ses'
 * will not set 'error' field
 * returns <0 on error, 0 on ok
 */
extern int samGenerateKeys (Sam3Session *ses, const char *hostname, int port);


////////////////////////////////////////////////////////////////////////////////
/*
 * sends datagram to 'destkey' endpoint
 * 'destkey' is 516-byte public key
 * returns <0 on error, 0 on ok
 * you still have to call samCloseSession() on failure
 * sets ses->error on error
 * don't send datagrams bigger than 31KB!
 */
extern int sam3DatagramSend (Sam3Session *ses, const char *hostname, int port, const char *destkey, const void *buf, int bufsize);

/*
 * receives datagram and sets 'destkey' to source pubkey (if not RAW)
 * returns <0 on error (buffer too small is error too) or number of bytes written to 'buf'
 * you still have to call samCloseSession() on failure
 * sets ses->error on error
 * will necer receive datagrams bigger than 31KB (32KB for RAW)
 */
extern int sam3DatagramReceive (Sam3Session *ses, void *buf, int bufsize);


////////////////////////////////////////////////////////////////////////////////
extern int sam3GenChannelName (char *dest, int minlen, int maxlen);


#ifdef __cplusplus
}
#endif
#endif
