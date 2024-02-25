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
#ifndef LIBSAM3_H
#define LIBSAM3_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#undef ssize_t
#ifdef _WIN64
typedef signed int64 ssize_t;
typedef int ssize_t;
#endif /* _WIN64 */
#endif /* _SSIZE_T_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////
extern int libsam3_debug;

////////////////////////////////////////////////////////////////////////////////
#define SAM3_HOST_DEFAULT (NULL)
#define SAM3_PORT_DEFAULT (0)

#define SAM3_DESTINATION_TRANSIENT (NULL)

#define SAM3_PUBKEY_SIZE (516)
#define SAM3_CERT_SIZE (100)
#define SAM3_PRIVKEY_MIN_SIZE (884)
#define SAM3_PRIVKEY_MAX_SIZE (1024)

////////////////////////////////////////////////////////////////////////////////
/* returns fd or -1 */
/* 'ip': host IP; can be NULL */
extern int sam3tcpConnect(const char *hostname, int port, uint32_t *ip);
extern int sam3tcpConnectIP(uint32_t ip, int port);

/* <0: error; 0: ok */
extern int sam3tcpDisconnect(int fd);

/* <0: error; 0: ok */
extern int sam3tcpSetTimeoutSend(int fd, int timeoutms);

/* <0: error; 0: ok */
extern int sam3tcpSetTimeoutReceive(int fd, int timeoutms);

/* <0: error; 0: ok */
/* sends the whole buffer */
extern int sam3tcpSend(int fd, const void *buf, size_t bufSize);

/* <0: received (-res) bytes; read error */
/* can return less that requesten bytes even if `allowPartial` is 0 when
 * connection is closed */
extern ssize_t sam3tcpReceiveEx(int fd, void *buf, size_t bufSize,
                                int allowPartial);

extern ssize_t sam3tcpReceive(int fd, void *buf, size_t bufSize);

extern int sam3tcpPrintf(int fd, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

extern int sam3tcpReceiveStr(int fd, char *dest, size_t maxSize);

/* pass NULL for 'localhost' and 0 for 7655 */
/* 'ip': host IP; can be NULL */
extern int sam3udpSendTo(const char *hostname, int port, const void *buf,
                         size_t bufSize, uint32_t *ip);
extern int sam3udpSendToIP(uint32_t ip, int port, const void *buf,
                           size_t bufSize);

////////////////////////////////////////////////////////////////////////////////
typedef struct SAMFieldList {
  char *name;
  char *value;
  struct SAMFieldList *next;
} SAMFieldList;

extern void sam3FreeFieldList(SAMFieldList *list);
extern void sam3DumpFieldList(const SAMFieldList *list);

/* read and parse SAM reply */
/* NULL: error; else: list of fields */
/* first item is always 2-word reply, with first word in name and second in
 * value */
extern SAMFieldList *sam3ReadReply(int fd);

extern SAMFieldList *sam3ParseReply(const char *rep);

/*
 * example:
 *   r0: 'HELLO'
 *   r1: 'REPLY'
 *   field: NULL or 'RESULT'
 *   VALUE: NULL or 'OK'
 * returns bool
 */
extern int sam3IsGoodReply(const SAMFieldList *list, const char *r0,
                           const char *r1, const char *field,
                           const char *value);

extern const char *sam3FindField(const SAMFieldList *list, const char *field);

////////////////////////////////////////////////////////////////////////////////
/* pass NULL for 'localhost' and 0 for 7656 */
/* returns <0 on error or socket fd on success */
extern int sam3Handshake(const char *hostname, int port, uint32_t *ip);
extern int sam3HandshakeIP(uint32_t ip, int port);

////////////////////////////////////////////////////////////////////////////////
typedef enum {
  SAM3_SESSION_RAW,
  SAM3_SESSION_DGRAM,
  SAM3_SESSION_STREAM
} Sam3SessionType;

typedef enum {
  DSA_SHA1,
  ECDSA_SHA256_P256,
  ECDSA_SHA384_P384,
  ECDSA_SHA512_P521,
  EdDSA_SHA512_Ed25519
} Sam3SigType;

typedef struct Sam3Session {
  Sam3SessionType type;
  Sam3SigType sigType;
  int fd;
  char privkey[SAM3_PRIVKEY_MAX_SIZE + 1]; // destination private key (asciiz)
  char pubkey[SAM3_PUBKEY_SIZE + SAM3_CERT_SIZE +
              1];   // destination public key (asciiz)
  char channel[66]; // name of this sam session (asciiz)
  char destkey[SAM3_PUBKEY_SIZE + SAM3_CERT_SIZE +
               1]; // for DGRAM sessions (asciiz)
  // int destsig;
  char error[32]; // error message (asciiz)
  uint32_t ip;
  int port; // this will be changed to UDP port for DRAM/RAW (can be 0)
  struct Sam3Connection *connlist; // list of opened connections
  int fwd_fd;
  bool silent;
} Sam3Session;

typedef struct Sam3Connection {
  Sam3Session *ses;
  struct Sam3Connection *next;
  int fd;
  char destkey[SAM3_PUBKEY_SIZE + SAM3_CERT_SIZE +
               1]; // remote destination public key (asciiz)
  int destcert;
  char error[32]; // error message (asciiz)
} Sam3Connection;

////////////////////////////////////////////////////////////////////////////////
/*
 * create SAM session
 * pass NULL as hostname for 'localhost' and 0 as port for 7656
 * pass NULL as privkey to create TRANSIENT session
 * 'params' can be NULL
 * see http://www.i2p2.i2p/i2cp.html#options for common options,
 * and http://www.i2p2.i2p/streaming.html#options for STREAM options
 * if result<0: error, 'ses' fields are undefined, no need to call
 * sam3CloseSession() if result==0: ok, all 'ses' fields are filled
 * TODO: don't clear 'error' field on error (and set it to something meaningful)
 */
extern int sam3CreateSession(Sam3Session *ses, const char *hostname, int port,
                             const char *privkey, Sam3SessionType type,
                             Sam3SigType sigType, const char *params);

/*
 * create SAM session with SILENT=True
 * pass NULL as hostname for 'localhost' and 0 as port for 7656
 * pass NULL as privkey to create TRANSIENT session
 * 'params' can be NULL
 * see http://www.i2p2.i2p/i2cp.html#options for common options,
 * and http://www.i2p2.i2p/streaming.html#options for STREAM options
 * if result<0: error, 'ses' fields are undefined, no need to call
 * sam3CloseSession() if result==0: ok, all 'ses' fields are filled
 * TODO: don't clear 'error' field on error (and set it to something meaningful)
 */
extern int sam3CreateSilentSession(Sam3Session *ses, const char *hostname,
                                   int port, const char *privkey,
                                   Sam3SessionType type, Sam3SigType sigType,
                                   const char *params);

/*
 * close SAM session (and all it's connections)
 * returns <0 on error, 0 on ok
 * 'ses' must be properly initialized
 */
extern int sam3CloseSession(Sam3Session *ses);

/*
 * check to see if a SAM session is silent and output
 * characters for use with sam3tcpPrintf() checkIsSilent
 */

const char *checkIsSilent(Sam3Session *ses);

/*
 * Check to make sure that the destination in use is of a valid length, returns
 * 1 if true and 0 if false.
 */
int sam3CheckValidKeyLength(const char *pubkey);

/*
 * open stream connection to 'destkey' endpoint
 * 'destkey' is 516-byte public key (asciiz)
 * returns <0 on error, fd on ok
 * you still have to call sam3CloseSession() on failure
 * sets ses->error on error
 */
extern Sam3Connection *sam3StreamConnect(Sam3Session *ses, const char *destkey);

/*
 * accepts stream connection and sets 'destkey'
 * 'destkey' is 516-byte public key
 * returns <0 on error, fd on ok
 * you still have to call sam3CloseSession() on failure
 * sets ses->error on error
 * note that there is no timeouts for now, but you can use sam3tcpSetTimeout*()
 */
extern Sam3Connection *sam3StreamAccept(Sam3Session *ses);

/*
 * sets up forwarding stream connection
 * returns <0 on error, 0 on ok
 * you still have to call sam3CloseSession() on failure
 * sets ses->error on error
 * note that there is no timeouts for now, but you can use sam3tcpSetTimeout*()
 */
extern int sam3StreamForward(Sam3Session *ses, const char *hostname, int port);

/*
 * close SAM connection
 * returns <0 on error, 0 on ok
 * 'conn' must be properly initialized
 * 'conn' is invalid after call
 */
extern int sam3CloseConnection(Sam3Connection *conn);

////////////////////////////////////////////////////////////////////////////////
/*
 * generate new keypair
 * fills 'privkey' and 'pubkey' only
 * you should not call sam3CloseSession() on 'ses'
 * will not set 'error' field
 * returns <0 on error, 0 on ok
 */
extern int sam3GenerateKeys(Sam3Session *ses, const char *hostname, int port,
                            int sigType);

/*
 * do name lookup (something like gethostbyname())
 * fills 'destkey' only
 * you should not call sam3CloseSession() on 'ses'
 * will set 'error' field
 * returns <0 on error, 0 on ok
 */
extern int sam3NameLookup(Sam3Session *ses, const char *hostname, int port,
                          const char *name);

////////////////////////////////////////////////////////////////////////////////
/*
 * sends datagram to 'destkey' endpoint
 * 'destkey' is 516-byte public key
 * returns <0 on error, 0 on ok
 * you still have to call sam3CloseSession() on failure
 * sets ses->error on error
 * don't send datagrams bigger than 31KB!
 */
extern int sam3DatagramSend(Sam3Session *ses, const char *destkey,
                            const void *buf, size_t bufsize);

/*
 * receives datagram and sets 'destkey' to source pubkey (if not RAW)
 * returns <0 on error (buffer too small is error too) or number of bytes
 * written to 'buf' you still have to call sam3CloseSession() on failure sets
 * ses->error on error will necer receive datagrams bigger than 31KB (32KB for
 * RAW)
 */
extern ssize_t sam3DatagramReceive(Sam3Session *ses, void *buf, size_t bufsize);

/*
 * generate random sam channel name
 * return the size of the string
 */
extern size_t sam3GenChannelName(char *dest, size_t minlen, size_t maxlen);

////////////////////////////////////////////////////////////////////////////////
// NOT including '\0' terminator
static inline size_t sam3Base32EncodedLength(size_t size) {
  return (((size + 5 - 1) / 5) * 8);
}

// output 8 bytes for every 5 input
// return size or <0 on error
extern ssize_t sam3Base32Encode(char *dest, size_t destsz, const void *srcbuf,
                                size_t srcsize);

#ifdef __cplusplus
}
#endif
#endif
