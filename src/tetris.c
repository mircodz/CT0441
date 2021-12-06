#include "tetris.h"

int
calculate_score(int rows, int n)
{
  switch (rows) {
  case 1: return 40 * (n + 1);
  case 2: return 100 * (n + 1);
  case 3: return 300 * (n + 1);
  case 4: return 1200 * (n + 1);
  default: return 0;
  }
}

int
level_to_timesteps(int level)
{
  const static int frames[30] = {48, 43, 38, 33, 28, 23, 18, 13, 8, 6, 5, 5, 5, 4, 4,
                                 4,  3,  3,  3,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 1};
  return frames[min(level, 30)]; /* caps at 30 */
}

color_t
tetramino_to_color(int t)
{
  const color_t ts[] = {
      color_cyan,
      color_yellow,
      color_purple,
      color_green,
      color_red,
      color_blue,
      color_orange,
  };
  /* shadow tetraminos will use the same colors as the normal ones */
  return ts[(t - 1) % 7];
}
