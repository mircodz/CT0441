#include "game_state.h"

#include <dz/utf8.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "collision.h"
#include "field.h"
#include "tetris.h"

game_state *
game_state_new()
{
  game_state *gs = malloc(sizeof(game_state));

  gs->field = field_new(10, 15);
  gs->mask  = field_new(10, 15);

  for (int i = 0; i < NEXT_QUEUE_SIZE; i++) {
    gs->next[i] = new_random_tetramino();
  }

  gs->holding.size = 0;

  gs->has_held      = false;
  gs->level         = 0;
  gs->total_cleared = 0;
  gs->score         = 0;

  gs->last_tick = 0;

  return gs;
}

void
game_state_free(game_state *gs)
{
  field_free(gs->field);
  field_free(gs->mask);
  free(gs);
}

void
game_state_step_pieces(game_state *gs)
{
  gs->active = tetramino_copy(gs->next[0]);

  gs->at.x = 0;
  gs->at.y = 0;

  for (int i = 0; i < NEXT_QUEUE_SIZE - 1; i++) {
    gs->next[i] = gs->next[i + 1];
  }
  gs->next[NEXT_QUEUE_SIZE - 1] = new_random_tetramino();

  gs->has_held = false;

  int cleared = field_cleanup(gs->field);
  gs->score += calculate_score(cleared, gs->level);
  gs->total_cleared += cleared;
  gs->level = gs->total_cleared / 10;
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
    game_state_step_pieces(gs);
  } else {
    tetramino_t tmp;
    tmp         = gs->holding;
    gs->holding = gs->active;
    gs->active  = tmp;
  }
  gs->at.x     = 0;
  gs->at.y     = 0;
  gs->has_held = true;

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
  game_state_step_pieces(gs);
  gs->at.x = 0;
  gs->at.y = 0;
}

bool
game_state_tick(game_state *gs, long dt)
{
  gs->last_tick += dt;
  if (gs->last_tick > 1000000 / 30 * level_to_timesteps(gs->level)) {
    if (check_collision(gs->field, gs->active, TP(gs->at, 0, 1))) {
      place_piece(gs->field, gs->active, gs->at);
      game_state_step_pieces(gs);
      gs->at.x = 0;
      gs->at.y = 0;
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
  for (int i = 0; i < f->w; i++) {
    for (int j = 0; j < f->h; j++) {
      const int v = f->data[j * f->w + i];
      if (v > 0) {
        const color_t color = tetramino_to_color(v);
        const rect_t  rect  = (rect_t){
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
  for (int i = 0; i < t.size; i++) {
    for (int j = 0; j < t.size; j++) {
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
  point_t shadow_at = find_shadow(gs->field, gs->active, gs->at);
  memset(gs->mask->data, 0, sizeof(int) * 150);

  place_shadow(gs->mask, gs->active, shadow_at);
  place_piece(gs->mask, gs->active, gs->at);

  char buf_score[128];
  int  len_score = snprintf(buf_score, 128, "Score: %d", gs->score);

  char buf_cleared[128];
  int  len_cleared = snprintf(buf_cleared, 128, "Cleared: %d", gs->total_cleared);

  char buf_level[128];
  int  len_level = snprintf(buf_level, 128, "Level: %d", gs->level);

  draw_field(e->screen, (rect_t){.x = 15, .y = 1}, gs->mask);
  draw_field(e->screen, (rect_t){.x = 15, .y = 1}, gs->field);

  point_t at_stats = {2, 19};

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
  for (int i = 0; i < NEXT_QUEUE_SIZE; i++) {
    draw_tetramino(e->screen, (rect_t){.x = 40, .y = 2 + i * 4}, gs->next[i]);
  }
}

void
game_state_draw_dont_compute(game_state *gs, engine_t *e)
{
  /*
  point_t shadow_at = find_shadow(gs->field, gs->active, gs->at);
  memset(gs->mask->data, 0, sizeof(int) * 150);

  place_shadow(gs->mask, gs->active, shadow_at);
  place_piece(gs->mask, gs->active, gs->at);


  draw_field(e->screen, (rect_t){.x = 15, .y = 1}, gs->mask);
  */

  char buf_score[128];
  int  len_score = snprintf(buf_score, 128, "Score: %d", gs->score);

  char buf_cleared[128];
  int  len_cleared = snprintf(buf_cleared, 128, "Cleared: %d", gs->total_cleared);

  char buf_level[128];
  int  len_level = snprintf(buf_level, 128, "Level: %d", gs->level);

  draw_field(e->screen, (rect_t){.x = 75, .y = 1}, gs->field);

  point_t at_stats = {2 + 60, 19};

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

  /* hold tetramino */
  // draw_tetramino(e->screen, (rect_t){.x = 3, .y = 2}, gs->holding);

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
  // for (int i = 0; i < NEXT_QUEUE_SIZE; i++) {
  //   draw_tetramino(e->screen, (rect_t){.x = 40, .y = 2 + i * 4}, gs->next[i]);
  // }
}

void
game_state_from_data(game_state *gs, int *data, int *ts)
{
  for (int i = 0; i < 150; i++) {
    gs->field->data[i] = data[i];
  }
}
