/* async SAMv3 library
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 * I2P-Bote: 5m77dFKGEq6~7jgtrfw56q3t~SmfwZubmGdyOLQOPoPp8MYwsZ~pfUCwud6LB1EmFxkm4C3CGlzq-hVs9WnhUV
 * we are the Borg. */
#ifndef LIBSAM3A_H
#define LIBSAM3A_H

#include <stdint.h>
#include <time.h>

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


////////////////////////////////////////////////////////////////////////////////
extern int libsam3a_debug;


////////////////////////////////////////////////////////////////////////////////
#define SAM3A_HOST_DEFAULT  (NULL)
#define SAM3A_PORT_DEFAULT  (0)

#define SAM3A_DESTINATION_TRANSIENT  (NULL)

#define SAM3A_PUBKEY_SIZE   (516)
#define SAM3A_PRIVKEY_SIZE  (884)


////////////////////////////////////////////////////////////////////////////////
extern uint64_t sam3atimeval2ms (const struct timeval *tv);
extern void sam3ams2timeval (struct timeval *tv, uint64_t ms);


////////////////////////////////////////////////////////////////////////////////
typedef struct Sam3ASession Sam3ASession;
typedef struct Sam3AConnection Sam3AConnection;


typedef enum {
  SAM3A_SESSION_RAW,
  SAM3A_SESSION_DGRAM,
  SAM3A_SESSION_STREAM
} Sam3ASessionType;


typedef struct {
  char *data;
  int dataSize;
  int dataUsed;
  int dataPos;
  void *udata;
  union {
    void (*cbReplyCheckSes)(Sam3ASession *ses);
    void (*cbReplyCheckConn)(Sam3AConnection *conn);
  };
} Sam3AAsyncIO;


typedef struct {
  void (*cbError) (Sam3ASession *ses);
  void (*cbCreated) (Sam3ASession *ses); // successfull SESSION CREATE
  void (*cbDisconnected) (Sam3ASession *ses); // or closed; will called only after cbCreated()
  void (*cbDatagramRead) (Sam3ASession *ses, const void *buf, int bufsize); // got datagram; bufsize >= 0; destkey set
  //
  void (*cbDestroy) (Sam3ASession *ses); // fd already closed, but keys is not cleared
} Sam3ASessionCallbacks;


struct Sam3ASession {
  Sam3ASessionType type;
  int fd;
  int cancelled; // fd was shutdown()ed, but not closed yet
  char privkey[SAM3A_PRIVKEY_SIZE+1]; // (asciiz)
  char pubkey[SAM3A_PUBKEY_SIZE+1]; // (asciiz)
  char channel[66]; // (asciiz)
  char destkey[SAM3A_PUBKEY_SIZE+1]; // for DGRAM sessions (asciiz)
  char error[64]; // (asciiz)
  uint32_t ip;
  int port; // this will be changed to UDP port for DRAM/RAW (can be 0)
  Sam3AConnection *connlist; // list of opened connections
  // for async i/o
  Sam3AAsyncIO aio;
  void (*cbAIOProcessorR) (Sam3ASession *ses); // internal
  void (*cbAIOProcessorW) (Sam3ASession *ses); // internal
  int callDisconnectCB;
  char *params; // will be cleared only by sam3aCloseSession()
  int timeoutms;
  //
  Sam3ASessionCallbacks cb;
  void *udata;
};


typedef struct {
  void (*cbError) (Sam3AConnection *ct);
  void (*cbDisconnected) (Sam3AConnection *ct); // or closed; only after cbConnected()/cbAccepted()
  void (*cbConnected) (Sam3AConnection *ct);
  void (*cbAccepted) (Sam3AConnection *ct);
  void (*cbSent) (Sam3AConnection *ct); // data sent, can add new data; will be called after connect/accept
  void (*cbRead) (Sam3AConnection *ct, const void *buf, int bufsize); // data read (bufsize is always > 0)
  //
  void (*cbDestroy) (Sam3AConnection *ct); // fd already closed, but keys is not cleared
} Sam3AConnectionCallbacks;


struct Sam3AConnection {
  Sam3ASession *ses;
  Sam3AConnection *next;
  int fd;
  int cancelled; // fd was shutdown()ed, but not closed yet
  char destkey[SAM3A_PUBKEY_SIZE+1]; // (asciiz)
  char error[32]; // (asciiz)
  // for async i/o
  Sam3AAsyncIO aio;
  void (*cbAIOProcessorR) (Sam3AConnection *ct); // internal
  void (*cbAIOProcessorW) (Sam3AConnection *ct); // internal
  int callDisconnectCB;
  //
  Sam3AConnectionCallbacks cb;
  void *udata;
};


////////////////////////////////////////////////////////////////////////////////
/*
 * check if session is active (i.e. have opened socket)
 * returns bool
 */
extern int sam3aIsActiveSession (const Sam3ASession *ses);

/*
 * check if connection is active (i.e. have opened socket)
 * returns bool
 */
extern int sam3aIsActiveConnection (const Sam3AConnection *conn);


////////////////////////////////////////////////////////////////////////////////
/*
 * note, that return error codes indicates invalid structure, pointer or fd
 * (i.e. immediate errors); all network errors indicated with cbError() callback
 */

/*
 * create SAM session
 * pass NULL as hostname for 'localhost' and 0 as port for 7656
 * pass NULL as privkey to create TRANSIENT session
 * 'params' can be NULL
 * see http://www.i2p2.i2p/i2cp.html#options for common options,
 * and http://www.i2p2.i2p/streaming.html#options for STREAM options
 * if result<0: error, 'ses' fields are undefined, no need to call sam3aCloseSession()
 * if result==0: ok, all 'ses' fields are filled
 * TODO: don't clear 'error' field on error (and set it to something meaningful)
 */
extern int sam3aCreateAsyncSessionEx (Sam3ASession *ses, const Sam3ASessionCallbacks *cb,
  const char *hostname, int port, const char *privkey, Sam3ASessionType type, const char *params, int timeoutms);

static inline int sam3aCreateAsyncSession (Sam3ASession *ses, const Sam3ASessionCallbacks *cb,
  const char *hostname, int port, const char *privkey, Sam3ASessionType type)
{
  return sam3aCreateAsyncSessionEx(ses, cb, hostname, port, privkey, type, NULL, -1);
}

/*
 * close SAM session (and all it's connections)
 * returns <0 on error, 0 on ok
 * 'ses' must be properly initialized
 */
extern int sam3aCloseSession (Sam3ASession *ses);

/*
 * cancel SAM session (and all it's connections), but don't free() or clear anything except fds
 * returns <0 on error, 0 on ok
 * 'ses' must be properly initialized
 */
extern int sam3aCancelSession (Sam3ASession *ses);

/*
 * open stream connection to 'destkey' endpoint
 * 'destkey' is 516-byte public key (asciiz)
 * returns <0 on error
 * sets ses->error on memory or socket creation error
 */
extern Sam3AConnection *sam3aStreamConnect (Sam3ASession *ses, const Sam3AConnectionCallbacks *cb, const char *destkey);

/*
 * accepts stream connection and sets 'destkey'
 * 'destkey' is 516-byte public key
 * returns <0 on error, fd on ok
 * you still have to call sam3aCloseSession() on failure
 * sets ses->error on error
 * note that there is no timeouts for now, but you can use sam3atcpSetTimeout*()
 */
extern Sam3AConnection *sam3aStreamAccept (Sam3ASession *ses, const Sam3AConnectionCallbacks *cb);

/*
 * close SAM connection
 * returns <0 on error, 0 on ok
 * 'conn' must be properly initialized
 * 'conn' is invalid after call
 */
extern int sam3aCloseConnection (Sam3AConnection *conn);

/*
 * cancel SAM connection, but don't free() or clear anything except fd
 * returns <0 on error, 0 on ok
 * 'conn' must be properly initialized
 * 'conn' is invalid after call
 */
extern int sam3aCancelConnection (Sam3AConnection *conn);


////////////////////////////////////////////////////////////////////////////////
/*
 * send data
 * this function should be used in cbSent() callback
 * if it is used outside of callback, it can return positive non-zero number,
 * which indicates that sending queue is full; but it can queue some data
 * if there is space in queue
 * return: <0: error; 0: ok; >0: send queue not empty, can't send data
 */
extern int sam3aSend (Sam3AConnection *conn, const void *data, int datasize);

/*
 * sends datagram to 'destkey' endpoint
 * 'destkey' is 516-byte public key
 * returns <0 on error, 0 on ok
 * you still have to call sam3aCloseSession() on failure
 * sets ses->error on error
 * don't send datagrams bigger than 31KB!
 */
extern int sam3aDatagramSend (Sam3ASession *ses, const char *destkey, const void *buf, int bufsize);


////////////////////////////////////////////////////////////////////////////////
/*
 * generate random channel name
 * dest should be at least (maxlen+1) bytes big
 */
extern int sam3aGenChannelName (char *dest, int minlen, int maxlen);


////////////////////////////////////////////////////////////////////////////////
/*
 * generate new keypair
 * fills 'privkey' and 'pubkey' only
 * you should call sam3aCloseSession() on 'ses'
 * cbCreated callback will be called when keys generated
 * returns <0 on error, 0 on ok
 */
extern int sam3aGenerateKeysEx (Sam3ASession *ses, const Sam3ASessionCallbacks *cb, const char *hostname, int port,
  int timeoutms);

static inline int sam3aGenerateKeys (Sam3ASession *ses, const Sam3ASessionCallbacks *cb, const char *hostname, int port) {
  return sam3aGenerateKeysEx(ses, cb, hostname, port, -1);
}


/*
 * do name lookup (something like gethostbyname())
 * fills 'destkey' only
 * you should call sam3aCloseSession() on 'ses'
 * cbCreated callback will be called when keys generated, ses->destkey will be set
 * returns <0 on error, 0 on ok
 */
extern int sam3aNameLookupEx (Sam3ASession *ses, const Sam3ASessionCallbacks *cb, const char *hostname, int port,
  const char *name, int timeoutms);

static inline int sam3aNameLookup (Sam3ASession *ses, const Sam3ASessionCallbacks *cb, const char *hostname, int port,
  const char *name)
{
  return sam3aNameLookupEx(ses, cb, hostname, port, name, -1);
}


////////////////////////////////////////////////////////////////////////////////
/*
 * append session fd to read and write sets if necessary
 * adds all alive session connections too
 * returns maxfd or -1
 * TODO: should keep fd count so it will not exceed FD_SETSIZE!
 */
extern int sam3aAddSessionToFDS (Sam3ASession *ses, int maxfd, fd_set *rds, fd_set *wrs);

/*
 * process session i/o (and all session connections i/o)
 * should be called after successful select()
 */
extern void sam3aProcessSessionIO (Sam3ASession *ses, fd_set *rds, fd_set *wrs);


#ifdef __cplusplus
}
#endif
#endif
