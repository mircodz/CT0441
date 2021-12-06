#include "collision.h"

collision_type_e
check_collision(field_t *f, tetramino_t t, point_t at)
{
  int i;
  int j;
  for (i = 0; i < t.size; i++) {
    for (j = 0; j < t.size; j++) {
      const int vt = t.data[i * t.size + j];
      /* check lateral collision with left side */
      if (vt && at.x + j < 0) {
        return COLLISION_TYPE_LEFT;
      }
      /* check lateral collision with right side */
      if (vt && at.x + j > f->w - 1) {
        return COLLISION_TYPE_RIGHT;
      }
      /* check collision with the floor */
      if (vt && at.y + i == f->h) {
        return COLLISION_TYPE_DOWN;
      }
      /* clang-format off */
      if (at.y + i > f->h - 1
          || at.x + j > f->w - 1
          || (at.y + i) * f->w + at.x + j > (f->h) * f->w
          || (at.y + i) * f->w + at.x + j < 0) {
        continue;
      }
      /* clang-format on */
      /* check collision with other tetraminos in the field */
      const int vf = f->data[(at.y + i) * f->w + at.x + j];
      if (vf && vt) {
        return COLLISION_TYPE_DOWN;
      }
    }
  }
  return COLLISION_TYPE_NONE;
}

point_t
find_shadow(field_t *f, tetramino_t t, point_t cur)
{
  point_t at = cur;

  while (!check_collision(f, t, P(at, 0, 1))) {
    at.y++;
  }

  return at;
}
