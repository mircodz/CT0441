#include "tetramino.h"

#include <stdlib.h>

tetramino_t
tetramino_new(unsigned size)
{
  tetramino_t t;
  t.size = size;
  t.data = malloc(sizeof(int) * size * size);
  return t;
}

tetramino_t
tetramino_copy(tetramino_t other)
{
  tetramino_t t = tetramino_new(other.size);
  memcpy(t.data, other.data, sizeof(int) * other.size * other.size);
  return t;
}

void
rotate_tetramino(tetramino_t src, tetramino_t *dest)
{
  int i;
  int j;

  for (i = src.size - 1; i >= 0; i--) {
    for (j = 0; j < src.size; j++) {
      dest->data[j * src.size + src.size - i - 1] = src.data[i * src.size + j];
    }
  }

  dest->size = src.size;
}
