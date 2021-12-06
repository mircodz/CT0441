#ifndef __TETRAMINO_H__
#define __TETRAMINO_H__

typedef struct {
  int     *data;
  unsigned size;
} tetramino_t;

/**
  \brief Creates new tetramino, allocating new memory for `data` buffer.
  */
tetramino_t tetramino_new(unsigned size);

tetramino_t tetramino_copy(tetramino_t other);

void rotate_tetramino(tetramino_t src, tetramino_t *dest);

#endif /* __TETRAMINO_H__ */
