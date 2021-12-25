#ifndef __TETRAMINO_H__
#define __TETRAMINO_H__

typedef struct {
  int *data;
  int  size;
} tetramino_t;

extern int TETRAMINO1[];
extern int TETRAMINO2[];
extern int TETRAMINO3[];
extern int TETRAMINO4[];
extern int TETRAMINO5[];
extern int TETRAMINO6[];
extern int TETRAMINO7[];

extern const tetramino_t tetraminos[];
/**
  \brief Creates new tetramino, allocating new memory for `data` buffer.
  */
tetramino_t tetramino_new(int size);

tetramino_t tetramino_copy(tetramino_t other);

void rotate_tetramino(tetramino_t src, tetramino_t *dest);

tetramino_t new_random_tetramino();

#endif /* __TETRAMINO_H__ */
