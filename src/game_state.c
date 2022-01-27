/** Well... I'd argue this is 90% ANSI, and 10% a fucking mess... */

#include "game_state.h"

#include <dz/utf8.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "collision.h"
#include "field.h"
#include "client.h"
#include "tetris.h"

game_state *
game_state_new()
{
  int         i;
  game_state *gs = malloc(sizeof(game_state));

  gs->field = field_new(10, 15);
  gs->mask  = field_new(10, 15);

  for (i = 0; i < NEXT_QUEUE_SIZE; i++) {
    gs->next[i] = new_random_tetramino();
  }

  gs->holding.size = 0;

  gs->has_held      = false;
  gs->level         = 0;
  gs->total_cleared = 0;
  gs->score         = 0;

  gs->last_tick = 0;

  gs->at.x = 4;
  gs->at.y = 0;

  gs->game_over = false;

  return gs;
}

void
game_state_free(game_state *gs)
{
  field_free(gs->field);
  field_free(gs->mask);
  free(gs);
}

static bool
game_state_isgameover(game_state *gs) 
{
  return !check_collision(gs->field, gs->active, gs->at);
}

bool
game_state_step_pieces(game_state *gs)
{
  int i;
  int cleared;

  gs->active = tetramino_copy(gs->next[0]);

  gs->at.x = 4;
  gs->at.y = 0;

  for (i = 0; i < NEXT_QUEUE_SIZE - 1; i++) {
    gs->next[i] = gs->next[i + 1];
  }
  gs->next[NEXT_QUEUE_SIZE - 1] = new_random_tetramino();

  gs->has_held = false;

  cleared = field_cleanup(gs->field);
  gs->score += calculate_score(cleared, gs->level);
  gs->total_cleared += cleared;
  gs->level = gs->total_cleared / 10;

  return game_state_isgameover(gs) == COLLISION_TYPE_NONE;
}

void
game_state_drop_tetramino(game_state *gs)
{
  point_t at = find_shadow(gs->field, gs->active, gs->at);
  place_piece(gs->field, gs->active, at);
}

bool
game_state_try_swap(game_state *gs)
{
  if (gs->has_held) {
    return true;
  }
  if (gs->holding.size == 0) {
    gs->holding = gs->active;
  } else {
    tetramino_t tmp = gs->holding;
    gs->holding     = gs->active;
    gs->active      = tmp;
  }
  gs->at.x     = 4;
  gs->at.y     = 0;
  gs->has_held = true;

  gs->game_over = game_state_step_pieces(gs);

  return false;
}

bool
game_state_try_move_left(game_state *gs)
{
  gs->at.x++;
  if (check_collision(gs->field, gs->active, gs->at)) {
    gs->at.x--;
    return true;
  }
  return false;
}

bool
game_state_try_move_right(game_state *gs)
{
  gs->at.x--;
  if (check_collision(gs->field, gs->active, gs->at)) {
    gs->at.x++;
    return true;
  }
  return false;
}

bool
game_state_try_move_down(game_state *gs)
{
  gs->at.y++;
  if (check_collision(gs->field, gs->active, gs->at)) {
    gs->at.y--;
    return true;
  }
  return false;
}

bool
game_state_try_rotate(game_state *gs)
{
  tetramino_t out = tetramino_new(4);
  out.size        = 0;

  rotate_tetramino(gs->active, &out);
  switch (check_collision(gs->field, out, gs->at)) {
  case COLLISION_TYPE_LEFT:
    while (check_collision(gs->field, out, gs->at) == COLLISION_TYPE_LEFT) {
      gs->at.x++;
    }
    break;
  case COLLISION_TYPE_RIGHT:
    while (check_collision(gs->field, out, gs->at) == COLLISION_TYPE_RIGHT) {
      gs->at.x--;
    }
    break;
  case COLLISION_TYPE_DOWN:
  case COLLISION_TYPE_NONE: break;
  }
  gs->active = tetramino_copy(out);

  return false;
}

void
game_state_drop_piece(game_state *gs)
{
  game_state_drop_tetramino(gs);
  gs->at.x = 4;
  gs->at.y = 0;
  gs->game_over = game_state_step_pieces(gs);
}

bool
game_state_tick(game_state *gs, long dt)
{
  gs->last_tick += dt;
  if (gs->last_tick > 1000000 / 30 * level_to_timesteps(gs->level)) {
    if (check_collision(gs->field, gs->active, TP(gs->at, 0, 1))) {
      place_piece(gs->field, gs->active, gs->at);
      gs->at.x = 4;
      gs->at.y = 0;
      gs->game_over = game_state_step_pieces(gs);
    } else {
      gs->at.y++;
    }

    gs->last_tick = 0;
    return true;
  }
  return false;
}

static void
draw_field(screen_t *s, rect_t bb, field_t *f)
{
  int i;
  int j;
  for (i = 0; i < f->w; i++) {
    for (j = 0; j < f->h; j++) {
      const int v = f->data[j * f->w + i];
      if (v > 0) {
        const color_t color = tetramino_to_color(v);
        /* It's not ANSI, but at least it's const-correct... */
        const rect_t rect = (rect_t){
            .x     = bb.x + i * 2 + 1,
            .y     = bb.y + j + 1,
            .w     = 2,
            .h     = 1,
            .color = color,
        };
        if (v > 7) {
          screen_fill_rect(s, rect, TILESET_FULL_SHADOW1);
        } else {
          screen_fill_rect(s, rect, TILESET_FULL_BLOCKS);
        }
      }
    }
  }
  screen_draw_rect(s,
                   (rect_t){
                       .x     = bb.x,
                       .y     = bb.y,
                       .w     = f->w * 2 + 1,
                       .h     = f->h,
                       .color = color_white,
                   });
}

static void
draw_tetramino(screen_t *s, rect_t bb, tetramino_t t)
{
  int i;
  int j;
  for (i = 0; i < t.size; i++) {
    for (j = 0; j < t.size; j++) {
      const int v = t.data[j * t.size + i];
      if (v > 0) {
        screen_fill_rect(s,
                         (rect_t){
                             .x     = bb.x + i * 2 + 1,
                             .y     = bb.y + j + 1,
                             .w     = 2,
                             .h     = 1,
                             .color = tetramino_to_color(v),
                         },
                         TILESET_FULL_BLOCKS);
      }
    }
  }
}

void
game_state_draw_everything(game_state *gs, engine_t *e)
{
  static const point_t at_stats = {2, 19};

  char buf_score[128];
  char buf_cleared[128];
  char buf_level[128];
  int  len_score;
  int  len_cleared;
  int  len_level;

  point_t shadow_at;

  int i;

  shadow_at = find_shadow(gs->field, gs->active, gs->at);
  memset(gs->mask->data, 0, sizeof(int) * 150);

  place_shadow(gs->mask, gs->active, shadow_at);
  place_piece(gs->mask, gs->active, gs->at);

  len_score   = snprintf(buf_score, 128, "Score: %d", gs->score);
  len_cleared = snprintf(buf_cleared, 128, "Cleared: %d", gs->total_cleared);
  len_level   = snprintf(buf_level, 128, "Level: %d", gs->level);

  draw_field(e->screen, (rect_t){.x = 15, .y = 1}, gs->mask);
  draw_field(e->screen, (rect_t){.x = 15, .y = 1}, gs->field);

  screen_draw_text(e->screen,
                   (rect_t){
                       .x = at_stats.x,
                       .y = at_stats.y,
                       .w = 22,
                       .h = 1,
                   },
                   buf_score,
                   len_score,
                   color_white);

  screen_draw_text(e->screen,
                   (rect_t){
                       .x = at_stats.x,
                       .y = at_stats.y + 1,
                       .w = 22,
                       .h = 1,
                   },
                   buf_cleared,
                   len_cleared,
                   color_white);

  screen_draw_text(e->screen,
                   (rect_t){
                       .x = at_stats.x,
                       .y = at_stats.y + 2,
                       .w = 22,
                       .h = 1,
                   },
                   buf_level,
                   len_level,
                   color_white);

  screen_draw_rect(e->screen,
                   (rect_t){
                       .x     = 1,
                       .y     = 18,
                       .w     = 50,
                       .h     = 5,
                       .color = color_white,
                   });

  /* hold */
  screen_draw_rect(e->screen,
                   (rect_t){
                       .x     = 1,
                       .y     = 1,
                       .w     = 13,
                       .h     = 5,
                       .color = color_white,
                   });

  /* hold text */
  screen_draw_text(e->screen,
                   (rect_t){
                       .x = 2,
                       .y = 1,
                       .w = strlen("Holding"),
                       .h = 1,
                   },
                   "Holding",
                   strlen("Holding"),
                   color_white);

  /* hold tetramino */
  draw_tetramino(e->screen, (rect_t){.x = 3, .y = 2}, gs->holding);

  /* next rect */
  screen_draw_rect(e->screen,
                   (rect_t){
                       .x     = 38,
                       .y     = 1,
                       .w     = 13,
                       .h     = 2 + NEXT_QUEUE_SIZE * 4,
                       .color = color_white,
                   });

  /* next text */
  screen_draw_text(e->screen,
                   (rect_t){
                       .x = 39,
                       .y = 1,
                       .w = strlen("Next"),
                       .h = 1,
                   },
                   "Next",
                   strlen("Next"),
                   color_white);

  /* next tetraminos */
  for (i = 0; i < NEXT_QUEUE_SIZE; i++) {
    draw_tetramino(e->screen, (rect_t){.x = 40, .y = 2 + i * 4}, gs->next[i]);
  }
}

/**
  \TODO Reduce code duplication.
 */
void
game_state_draw_dont_compute(game_state *gs, engine_t *e)
{
  static const point_t at_stats = {2 + 60, 19}; /* position for stats screen */

  char buf_score[128];
  char buf_cleared[128];
  char buf_level[128];
  int  len_score;
  int  len_cleared;
  int  len_level;

  int i;

  len_score   = snprintf(buf_score, 128, "Score: %d", gs->score);
  len_cleared = snprintf(buf_cleared, 128, "Cleared: %d", gs->total_cleared);
  len_level   = snprintf(buf_level, 128, "Level: %d", gs->level);

  draw_field(e->screen, (rect_t){.x = 75, .y = 1}, gs->field);

  screen_draw_text(e->screen,
                   (rect_t){
                       .x = at_stats.x,
                       .y = at_stats.y,
                       .w = 22,
                       .h = 1,
                   },
                   buf_score,
                   len_score,
                   color_white);

  screen_draw_text(e->screen,
                   (rect_t){
                       .x = at_stats.x,
                       .y = at_stats.y + 1,
                       .w = 22,
                       .h = 1,
                   },
                   buf_cleared,
                   len_cleared,
                   color_white);

  screen_draw_text(e->screen,
                   (rect_t){
                       .x = at_stats.x,
                       .y = at_stats.y + 2,
                       .w = 22,
                       .h = 1,
                   },
                   buf_level,
                   len_level,
                   color_white);

  screen_draw_rect(e->screen,
                   (rect_t){
                       .x     = 1 + 60,
                       .y     = 18,
                       .w     = 50,
                       .h     = 5,
                       .color = color_white,
                   });

  /* hold */
  screen_draw_rect(e->screen,
                   (rect_t){
                       .x     = 1 + 60,
                       .y     = 1,
                       .w     = 13,
                       .h     = 5,
                       .color = color_white,
                   });

  /* hold text */
  screen_draw_text(e->screen,
                   (rect_t){
                       .x = 2 + 60,
                       .y = 1,
                       .w = strlen("Holding"),
                       .h = 1,
                   },
                   "Holding",
                   strlen("Holding"),
                   color_white);

  /* next rect */
  screen_draw_rect(e->screen,
                   (rect_t){
                       .x     = 38 + 60,
                       .y     = 1,
                       .w     = 13,
                       .h     = 2 + NEXT_QUEUE_SIZE * 4,
                       .color = color_white,
                   });

  /* next text */
  screen_draw_text(e->screen,
                   (rect_t){
                       .x = 39 + 60,
                       .y = 1,
                       .w = strlen("Next"),
                       .h = 1,
                   },
                   "Next",
                   strlen("Next"),
                   color_white);

  /* next tetraminos */
  for (i = 0; i < NEXT_QUEUE_SIZE; i++) {
    draw_tetramino(e->screen, (rect_t){.x = 40 + 60, .y = 2 + i * 4}, gs->next[i]);
  }
}
