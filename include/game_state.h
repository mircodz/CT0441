#ifndef __GAME_STATE_H__
#define __GAME_STATE_H__

#include <dz/common.h>
#include <dz/engine.h>
#include <dz/point.h>

#define NEXT_QUEUE_SIZE 3

#include "field.h"
#include "tetramino.h"

typedef struct {
  point_t at;

  field_t *field;
  field_t *mask;

  tetramino_t active;
  tetramino_t next[NEXT_QUEUE_SIZE];
  tetramino_t holding;

  bool has_held;
  int  level;
  int  total_cleared;
  int  score;

  long last_tick;
} game_state;

game_state *game_state_new();
void        game_state_free(game_state *gs);

/**
  \brief Pop tetramino from queue and set it as the active one.
 */
void game_state_step_pieces(game_state *gs);

/**
  \brief Move tetramino to the bottom of the field.
 */
void game_state_drop_tetramino(game_state *gs);

/**
  \brief Swap current tetramino with held one.
  \returns True if player cannot swap held piece.
  */
bool game_state_try_swap(game_state *gs);

bool game_state_try_move_left(game_state *gs);

bool game_state_try_move_right(game_state *gs);

bool game_state_try_move_down(game_state *gs);

bool game_state_try_rotate(game_state *gs);

void game_state_drop_piece(game_state *gs);

bool game_state_tick(game_state *gs, long dt);

void game_state_draw_everything(game_state *gs, engine_t *e);

void game_state_draw_dont_compute(game_state *gs, engine_t *e);

void game_state_from_data(game_state *gs, int *field, int *ts);

#endif /* __GAME_STATE_H__ */
