#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <dz/common.h>

/* there's a #include "game_state.h" a dozen lines under this */

#define CMD_AUTH     "AUTH"
#define CMD_JOIN     "JOIN"
#define CMD_LISTP    "LISTP\r\n"
#define CMD_LISTG    "LISTG\r\n"
#define CMD_GETSTATE "GETSTATE\r\n"
#define CMD_SETSTATE "SETSTATE"

#define CODE_AUTHENTICATION_SUCCESFULL 235
#define CODE_JOIN_SUCCESFULL           236
#define CODE_SETSTATE_SUCCESFULL       237

/**
  \todo Move to libdz.
  \todo Implement field parsing in libdz.

  \code
    // reads until delimiters, allocating space
    char *field = parse_field(delims = " \n");
  \endcode

  \todo Implement state machine similar to the one in the backend.

  \todo Implement protocol reflection support. I'll wait for the C++ project to work on this...
  \code
    reflect()
      res = send(client, "PROTOCOL");
      for cmd in res
        cmds.append(command::new{cmd})

    send_cmd(cmd)
      if cmd in cmds and check_validity(cmd)
        send(client, cmd)
  \endcode
  */
typedef struct {
  int sockfd;
} client_t;

/* required here to resolve circular import */
#include "game_state.h"

typedef enum {
  NotAuthenticated,
  Authenticated,
  Waiting,
  Playing,
} player_state_e;

typedef struct {
  char    *name;
  unsigned name_len;

  player_state_e state;

  char    *room;
  unsigned room_len;
} listp_entry_t;

typedef struct {
  listp_entry_t *players;
  unsigned       num;
} listp_t;

typedef struct {
  char    *room;
  unsigned room_len;

  char    *player_1;
  unsigned player_1_len;

  char    *player_2;
  unsigned player_2_len;
} listg_entry_t;

typedef struct {
  listg_entry_t *rooms;
  unsigned       num;
} listg_t;

/** \todo Allow for variable sized fields.
    Might want to implement a `capabilities` or `options` message to negotiate such options.
*/
typedef struct {
  int field[150];
  /* [holding, queued 1, queued 2, queued 3, ...] */
  int tetraminos[1 + NEXT_QUEUE_SIZE];
} getstate_t;

/** \todo Move to libdz. */
client_t *client_new(char *addr, int port);

/** \todo Move to libdz. */
void client_close(client_t *c);

/**
  \brief Autheticate to server with username \p name.

  \param name Username.
  \param len Username length.
  */
bool tetris_client_auth(client_t *c, char *name, int len);

/**
  \brief Join room of name \p name.

  \param name Room name.
  \param len Room name length.
  */
bool tetris_client_join(client_t *c, char *name, int len);

/**
  \brief Return result of "LISTP" command.
  \todo Make this safe. No way in hell this implementation is.

  Can only be executed after authentication.
  Caller must free the result though `listp_free`.
  */
listp_t *tetris_client_listp(client_t *c);

/**
  \see tertis_client_listp
  */
void listp_free(listp_t *l);

/**
  \brief Return result of "LISTG" command.
  \todo Make this safe. No way in hell this implementation is.

  Can only be executed after authentication.
  Caller must free the result though `listg_free`.
  */
listg_t *tetris_client_listg(client_t *c);

void listg_free(listg_t *l);

void tetris_client_getstate(client_t *c, getstate_t *out);

/**
  \brief Send current state to server.
  \returns True if success; false otherwise.

  \param data_f Field data.
  \param data_m Mask data.
  \param h Held tetramino.
  \param ts Tetramino queue.
  \param ts_len Length of tetramino queue.
  */
bool tetris_client_setstate(client_t *c, int *data_f, int *data_m, int h, int *ts, int ts_len);

#endif /* __CLIENT_H__ */
