#include <dz/engine.h>
#include <dz/mesh.h>
#include <dz/ui/button.h>
#include <dz/ui/statusbar.h>
#include <dz/ui/window.h>

#include "collision.h"
#include "field.h"
#include "tetramino.h"
#include "tetris.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  union {
    char *str;
    int   num;
    bool  val;
    void (*fn)();
  } data;

  char *title;

  enum {
    String,
    Integer,
    Checkbox,
  } type;
} entry_t;

typedef struct {
  rect_t   box;
  entry_t *entries;
} dialog_t;

entry_t entries[] = {
    (entry_t){.type = String, .title = "IP"},
    (entry_t){.type = Integer, .title = "Port"},
};

dialog_t dialog = {
    .box     = {0, 0, 10, 10},
    .entries = entries,
};

point_t at;

button_t *b;

/* clang-format off */

int TETRAMINO1[] = {
  0, 0, 0, 0,
  1, 1, 1, 1,
  0, 0, 0, 0,
  0, 0, 0, 0
};

int TETRAMINO2[] = {
  2, 2,
  2, 2,
};

int TETRAMINO3[] = {
  0, 0, 0,
  3, 3, 3,
  0, 3, 0,
};

int TETRAMINO4[] = {
  0, 4, 4,
  4, 4, 0,
  0, 0, 0,
};

int TETRAMINO5[] = {
  5, 5, 0,
  0, 5, 5,
  0, 0, 0,
};

int TETRAMINO6[] = {
  6, 0, 0,
  6, 6, 6,
  0, 0, 0,
};

int TETRAMINO7[] = {
  0, 0, 7,
  7, 7, 7,
  0, 0, 0,
};

/* clang-format on */

const tetramino_t tetraminos[] = {
    {TETRAMINO1, 4},
    {TETRAMINO2, 2},
    {TETRAMINO3, 3},
    {TETRAMINO4, 3},
    {TETRAMINO5, 3},
    {TETRAMINO6, 3},
    {TETRAMINO7, 3},
};

field_t *field;
field_t *mask;

#define NEXT_QUEUE_SIZE 3

tetramino_t active;
tetramino_t next[NEXT_QUEUE_SIZE];
tetramino_t holding;

bool has_held      = false;
int  level         = 0;
int  total_cleared = 0;
int  score         = 0;

void
step_pieces()
{
  int i;
  active = tetramino_copy(next[0]);

  at.x = 0;
  at.y = 0;

  for (i = 0; i < NEXT_QUEUE_SIZE - 1; i++) {
    next[i] = next[i + 1];
  }
  next[i]  = tetraminos[rand() % 7];
  has_held = false;

  int cleared = field_cleanup(field);
  score += calculate_score(cleared, level);
  total_cleared += cleared;
  level = total_cleared / 10;
}

void
place_shadow(field_t *f, tetramino_t t, int x, int y)
{
  int i;
  int j;
  for (i = 0; i < t.size; i++) {
    for (j = 0; j < t.size; j++) {
      if (t.data[i * t.size + j]) {
        field_at(f, j + x, i + y, t.data[i * t.size + j] + 7);
      }
    }
  }
}

void
place_piece(field_t *f, tetramino_t t, int x, int y)
{
  int i;
  int j;
  for (i = 0; i < t.size; i++) {
    for (j = 0; j < t.size; j++) {
      const int v = t.data[i * t.size + j];
      if (v) {
        field_at(f, j + x, i + y, v);
      }
    }
  }
}

void
drop_tetramino(field_t *f, tetramino_t t, point_t cur)
{
  point_t at = find_shadow(f, t, cur);
  place_piece(f, t, at.x, at.y);
}

void
draw_field(screen_t *s, rect_t bb, field_t *f)
{
  int i;
  int j;
  for (i = 0; i < f->w; i++) {
    for (j = 0; j < f->h; j++) {
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
  screen_draw_rect(
      s, (rect_t){.x = bb.x, .y = bb.y, .w = f->w * 2 + 1, .h = f->h, .color = color_white});
}

void
draw_tetramino(screen_t *s, rect_t bb, tetramino_t t)
{
  int i;
  int j;
  for (i = 0; i < t.size; i++) {
    for (j = 0; j < t.size; j++) {
      const int v = t.data[j * t.size + i];
      if (v > 0) {
        screen_fill_rect(s,
                         (rect_t){.x     = bb.x + i * 2 + 1,
                                  .y     = bb.y + j + 1,
                                  .w     = 2,
                                  .h     = 1,
                                  .color = tetramino_to_color(v)},
                         TILESET_FULL_BLOCKS);
      }
    }
  }
}

window_t    *w;
statusbar_t *sb;
tetramino_t out;

void
hook_keyboard(void *ptr, kb_event_t ev)
{
  engine_t *e = ptr;

  switch (ev.key) {
  case ' ':
    drop_tetramino(field, active, at);
    step_pieces();
    at.x = 0;
    at.y = 0;
    break;

  case KEY_UP:
    rotate_tetramino(active, &out);
    switch (check_collision(field, out, at)) {
    case COLLISION_TYPE_LEFT:
      while (check_collision(field, out, at) == COLLISION_TYPE_LEFT) {
        at.x++;
      }
      break;
    case COLLISION_TYPE_RIGHT:
      while (check_collision(field, out, at) == COLLISION_TYPE_RIGHT) {
        at.x--;
      }
      break;
    default: break;
    }
    active = tetramino_copy(out);
    break;

  case KEY_DOWN:
    at.y++;
    if (check_collision(field, active, at))
      at.y--;
    break;

  case KEY_LEFT:
    at.x++;
    if (check_collision(field, active, at))
      at.x--;
    break;

  case KEY_RIGHT:
    at.x--;
    if (check_collision(field, active, at))
      at.x++;
    break;

  case 'c':
    if (has_held) {
      break;
    }
    if (holding.size == 0) {
      holding = active;
      step_pieces();
    } else {
      tetramino_t tmp;
      tmp     = holding;
      holding = active;
      active  = tmp;
    }
    at.x     = 0;
    at.y     = 0;
    has_held = true;
    break;

  case 'q': e->run = false; break;
  }
}

void
hook_mouse(void *ptr, mouse_event_t ev)
{
  engine_t *e = ptr;

  button_check(b, ev.at);

  statusbar_fupdate(sb, "%d:%d", ev.at.x, ev.at.y);

  e->screen->buffer[ev.at.x + ev.at.y * e->screen->w] = (pixel_t){' ', {100, 100, 100}, {0, 0, 0}};
}

void
hook_loop(void *ptr, long dt)
{
  static long last = 0;
  engine_t   *e    = ptr;

  int i;

  last += dt;
  if (last > 1000000 / 30 * level_to_timesteps(level)) {
    if (check_collision(field, active, P(at, 0, 1))) {
      place_piece(field, active, at.x, at.y);
      step_pieces();
      at.x = 0;
      at.y = 0;
    } else {
      at.y++;
    }

    last = 0;
  }

  point_t shadow = find_shadow(field, active, at);

  memset(mask->data, 0, sizeof(int) * 150);

  place_shadow(mask, active, shadow.x, shadow.y);
  place_piece(mask, active, at.x, at.y);

  char buf_score[128];
  int  len_score = snprintf(buf_score, 128, "Score: %d", score);

  char buf_cleared[128];
  int  len_cleared = snprintf(buf_cleared, 128, "Cleared: %d", total_cleared);

  char buf_level[128];
  int  len_level = snprintf(buf_level, 128, "Level: %d", level);

  draw_field(e->screen, (rect_t){.x = 15, .y = 1}, mask);
  draw_field(e->screen, (rect_t){.x = 15, .y = 1}, field);

  statusbar_fupdate(sb, "%d", score);

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
                   (rect_t){.x = 2, .y = 1, .w = strlen("Holding"), .h = 1},
                   "Holding",
                   strlen("Holding"),
                   color_white);

  /* hold tetramino */
  draw_tetramino(e->screen, (rect_t){.x = 3, .y = 2}, holding);

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
                   (rect_t){.x = 39, .y = 1, .w = strlen("Next"), .h = 1},
                   "Next",
                   strlen("Next"),
                   color_white);

  /* next tetraminos */
  for (i = 0; i < NEXT_QUEUE_SIZE; i++) {
    draw_tetramino(e->screen, (rect_t){.x = 40, .y = 2 + i * 4}, next[i]);
  }

  button_draw(b, e->screen);
  window_draw(w, e->screen);
}

void
callback(void *)
{
  exit(1);
}

int
main()
{
  int i;
  int j;

  b = button_new(
      (rect_t){
          .x     = 35,
          .y     = 35,
          .w     = 10,
          .h     = 1,
          .color = color_white,
      },
      "Ok",
      callback);

  w = window_new(
      (rect_t){
          .x     = 30,
          .y     = 30,
          .w     = 20,
          .h     = 20,
          .color = color_white,
      },
      "Main Window");

  out          = tetramino_new(4);
  holding.size = 0;

  field = field_new(10, 15);
  mask  = field_new(10, 15);

  srand(time(0));

  next[0] = tetraminos[rand() % 7];
  next[1] = tetraminos[rand() % 7];
  next[2] = tetraminos[rand() % 7];

  step_pieces();
  place_piece(mask, active, at.x, at.y);

  int       ret;
  engine_t *e = engine_new(hook_keyboard, hook_mouse, hook_loop);

  sb = statusbar_new("diocane");

  ret = engine_loop(e);
  engine_free(e);

  statusbar_free(sb);
  return ret;
}
