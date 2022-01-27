/** A `dangling-pointer` fuck-fest. This entire file seems sketchy, but works fine. */

#include "tetramino.h"

#include <stdlib.h>

int TETRAMINO1[] = {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
int TETRAMINO2[] = {2, 2, 2, 2};
int TETRAMINO3[] = {0, 0, 0, 3, 3, 3, 0, 3, 0};
int TETRAMINO4[] = {0, 4, 4, 4, 4, 0, 0, 0, 0};
int TETRAMINO5[] = {5, 5, 0, 0, 5, 5, 0, 0, 0};
int TETRAMINO6[] = {6, 0, 0, 6, 6, 6, 0, 0, 0};
int TETRAMINO7[] = {0, 0, 7, 7, 7, 7, 0, 0, 0};

const tetramino_t tetraminos[7] = {
    {TETRAMINO1, 4},
    {TETRAMINO2, 2},
    {TETRAMINO3, 3},
    {TETRAMINO4, 3},
    {TETRAMINO5, 3},
    {TETRAMINO6, 3},
    {TETRAMINO7, 3},
};

tetramino_t
tetramino_new(int size)
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

tetramino_t
new_random_tetramino()
{
  return tetraminos[rand() % 7];
}
