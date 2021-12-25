#ifndef __FIELD_H__
#define __FIELD_H__

#include "tetramino.h"

#include <dz/point.h>

typedef struct {
  int *data;
  int  w;
  int  h;
} field_t;

field_t *field_new(int w, int h);

void field_free(field_t *f);

/**
  \brief Memory safe access to data of \p f
  */
void field_set_at(field_t *f, int x, int y, int v);

/**
  \brief Cleanup completed rows.
  \returns Number of cleared rows.
  */
int field_cleanup(field_t *f);

void field_shift(field_t *f, int row);

void place_piece(field_t *f, tetramino_t t, point_t at);

void place_shadow(field_t *f, tetramino_t t, point_t at);

#endif /* __FIELD_H__ */
