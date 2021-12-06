#ifndef __COLLISION_H__
#define __COLLISION_H__

#include <dz/common.h>
#include <dz/point.h>

#include "field.h"
#include "tetramino.h"

/**
 */
typedef enum {
  COLLISION_TYPE_NONE,
  COLLISION_TYPE_DOWN,
  COLLISION_TYPE_RIGHT,
  COLLISION_TYPE_LEFT,
} collision_type_e;

/**
  \brief Check whether the active tetramino should be halted.
  \param f Field, containing placed tetraminos.
  \param t Tetramino to be checked.
  \param at Tetramino's position.
  \see `collision_type_e`
  */
collision_type_e check_collision(field_t *f, tetramino_t t, point_t at);

/**
  \brief Calculate tetramino's shadow position.
  \param cur Current tetramino position.
  \returns Shadow's position.
  */
point_t find_shadow(field_t *f, tetramino_t t, point_t cur);

#endif /* __COLLISION_H__ */
