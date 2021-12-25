#include "client.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <dz/utf8.h>

player_state_e
chr_to_state(char c)
{
  switch (c) {
  case 'N': return NotAuthenticated;
  case 'A': return Authenticated;
  case 'W': return Waiting;
  case 'P': return Playing;
  }
}

#define MAX 2048

/**
  \see https://stackoverflow.com/a/28558742/6585097
  */
static ssize_t
read_until_crlf(int sockfd, char *p, size_t s)
{
  ssize_t bytes_read = 0;
  ssize_t result     = 0;
  int     read_cr    = 0;
  int     read_crlf  = 0;

  while (bytes_read < s) {
    result = recv(sockfd, p + bytes_read, 512, MSG_DONTWAIT);
    if (result == -1) {
      if ((EAGAIN == errno) || (EWOULDBLOCK == errno)) {
        continue;
      } else if (errno == EINTR) {
        break;
      } else {
        break;
      }
    } else if (result == 0) {
      break;
    }

    if (strstr(p, "\r\n")) {
      break;
    }

    bytes_read += result;
  }

  if (!read_crlf) {
    result = -1;
    errno  = ENOSPC;
  }

  return (0 >= result) ? result : bytes_read;
}

client_t *
client_new(char *addr, int port)
{
  client_t *c = malloc(sizeof(client_t));

  struct sockaddr_in servaddr;

  c->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (c->sockfd == -1) {
    return NULL;
  }

  bzero(&servaddr, sizeof(servaddr));

  servaddr.sin_family      = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(addr);
  servaddr.sin_port        = htons(port);

  if (connect(c->sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
    return NULL;
  }

  char buff[MAX];
  read_until_crlf(c->sockfd, buff, MAX); /* consume header */

  return c;
}

void
client_close(client_t *c)
{
  close(c->sockfd);
  free(c);
}

#define SEND_CMD(Cmd) write(c->sockfd, Cmd, sizeof(Cmd));

#define SEND_CMD_ARG_STR1(Cmd, Arg, Len)                                                                               \
  do {                                                                                                                 \
    char buff[256];                                                                                                    \
    Arg[Len] = '\0';                                                                                                   \
    int n    = snprintf(buff, 256, "%s %s\n", Cmd, Arg);                                                               \
    write(c->sockfd, buff, n);                                                                                         \
  } while (0);

bool
tetris_client_auth(client_t *c, char *name, int len)
{
  char buff[MAX];
  int  code;

  SEND_CMD_ARG_STR1(CMD_AUTH, name, len);

  bzero(buff, MAX);
  read_until_crlf(c->sockfd, buff, MAX);
  sscanf(buff, "%d", &code);

  return code == CODE_AUTHENTICATION_SUCCESFULL;
}

bool
tetris_client_join(client_t *c, char *name, int len)
{
  char buff[MAX];
  int  code;

  SEND_CMD_ARG_STR1(CMD_JOIN, name, len);

  bzero(buff, MAX);
  read_until_crlf(c->sockfd, buff, MAX);
  sscanf(buff, "%d", &code);

  return code == CODE_JOIN_SUCCESFULL;
}

listp_t *
tetris_client_listp(client_t *c)
{
  char     buff[MAX];
  int      code;
  unsigned n;

  SEND_CMD(CMD_LISTP);

  read_until_crlf(c->sockfd, buff, MAX);
  sscanf(buff, "%u", &n);

  listp_t *l = malloc(sizeof(listp_t));
  l->players = malloc(sizeof(listp_entry_t) * n);
  l->num     = n;

  for (int i = 0; i < n; i++) {
    char *s;

    bzero(buff, MAX);
    int r = read_until_crlf(c->sockfd, buff, MAX);

    /* parse player name */
    s                      = strchr(buff, ' ');
    l->players[i].name     = malloc(sizeof(char) * (s - buff) + 1);
    l->players[i].name_len = s - buff;
    memcpy(l->players[i].name, buff, l->players[i].name_len + 1);
    l->players[i].name[l->players[i].name_len] = '\0';

    /* parse state */
    l->players[i].state = chr_to_state(*(s + 1));

    /* parse room */
    s                      = strchr(s + 1, ' ');
    l->players[i].room     = malloc(sizeof(char) * (buff + r - s) + 1);
    l->players[i].room_len = buff + r - s - 2;
    memcpy(l->players[i].room, s + 1, l->players[i].room_len);
    l->players[i].room[l->players[i].room_len] = '\0';
  }

  return l;
}

void
listp_free(listp_t *l)
{
  for (int i = 0; i < l->num; i++) {
    free(l->players[i].name);
    free(l->players[i].room);
  }
  free(l->players);
  free(l);
}

listg_t *
tetris_client_listg(client_t *c)
{
  char     buff[MAX];
  int      code;
  unsigned n;

  SEND_CMD(CMD_LISTG);

  read_until_crlf(c->sockfd, buff, MAX);
  sscanf(buff, "%u", &n);

  listg_t *l = malloc(sizeof(listg_t));
  l->rooms   = malloc(sizeof(listg_entry_t) * n);
  l->num     = n;

  for (int i = 0; i < n; i++) {
    char *s;

    bzero(buff, MAX);
    int r = read_until_crlf(c->sockfd, buff, MAX);

    /* parse player name */
    s                    = strchr(buff, ' ');
    l->rooms[i].room     = malloc(sizeof(char) * (s - buff) + 1);
    l->rooms[i].room_len = s - buff;
    memcpy(l->rooms[i].room, buff, l->rooms[i].room_len + 1);
    l->rooms[i].room[l->rooms[i].room_len] = '\0';

    s                        = strchr(buff, ' ');
    l->rooms[i].player_1     = malloc(sizeof(char) * (s - buff) + 1);
    l->rooms[i].player_1_len = s - buff;
    memcpy(l->rooms[i].player_1, buff, l->rooms[i].player_1_len + 1);
    l->rooms[i].player_1[l->rooms[i].player_1_len] = '\0';

    l->rooms[i].player_2     = malloc(sizeof(char) * (buff + r - s) + 1);
    l->rooms[i].player_2_len = buff + r - s - 2;
    memcpy(l->rooms[i].player_2, s + 1, l->rooms[i].player_2_len);
    l->rooms[i].player_2[l->rooms[i].player_2_len] = '\0';
  }

  return l;
}

void
listg_free(listg_t *l)
{
  for (int i = 0; i < l->num; i++) {
    free(l->rooms[i].room);
    free(l->rooms[i].player_1);
    free(l->rooms[i].player_2);
  }
  free(l->rooms);
  free(l);
}

void
tetris_client_getstate(client_t *c, getstate_t *out)
{
  char buff[MAX];
  SEND_CMD(CMD_GETSTATE);
  read_until_crlf(c->sockfd, buff, MAX);

  int r = 0;
  for (int i = 0; i < 150; i++) {
    int pos;
    sscanf(buff + r, "%d%n", &out->field[i], &pos);
    r += pos;
  }
}

bool
tetris_client_setstate(client_t *c, int *data_f, int *data_m, int h, int *ts, int ts_len)
{
  write(c->sockfd, "SETSTATE ", sizeof("SETSTATE "));
  for (int i = 0; i < 150; i++) {
    char buff[16];
    int  n = snprintf(buff, 16, "%d ", data_f[i] | data_m[i]);
    write(c->sockfd, buff, n);
  }
  write(c->sockfd, "\r\n", sizeof("\r\n"));

  char buff[MAX];
  read_until_crlf(c->sockfd, buff, MAX);

  int code;
  sscanf(buff, "%d", &code);

  return code == CODE_SETSTATE_SUCCESFULL;
}
