#ifndef __TETRIS_H__
#define __TETRIS_H__

#include <dz/color.h>

#include "tetramino.h"

/**
  \brief Given the number of cleared rows and the current level calculate score.

  \param rows Number of cleared rows.
  \param n Current level.
  \see `https://tetris.fandom.com/wiki/Scoring`.
  */
int calculate_score(int rows, int n);

/**
  \Brief Return number of frames to wait before dropping the tetramino once more.

  \see `https://listfist.com/list-of-tetris-levels-by-speed-nes-ntsc-vs-pal`
  */
int level_to_timesteps(int level);

color_t tetramino_to_color(int t);

#endif /* __TETRIS_H__ */
