#include "field.h"

#include <stdlib.h>

field_t *
field_new(int w, int h)
{
  field_t *f = malloc(sizeof(field_t));
  f->data    = malloc(sizeof(int) * w * h);
  memset(f->data, 0, sizeof(int) * w * h);
  f->w = w;
  f->h = h;
  return f;
}

void
field_at(field_t *f, int x, int y, int v)
{
  if (y >= f->h || x >= f->w) {
    return;
  }
  f->data[y * f->w + x] = v;
}

int
field_cleanup(field_t *f)
{
  int i;
  int j;
  int rows = 0;
  for (i = f->h - 1; i >= 0; i--) {
    for (j = 0; j < f->w; j++) {
      if (!f->data[i * f->w + j]) {
        break;
      }
    }
    /* broke before end == line full */
    if (j == f->w) {
      field_shift(f, i);
      i += 1;
      rows += 1;
    }
  }
  return rows;
}

void
field_shift(field_t *f, int row)
{
  int i;
  for (i = row - 1; i >= 0; i--) {
    memcpy(f->data + (i + 1) * f->w, f->data + i * f->w, sizeof(int) * f->w);
  }
}
