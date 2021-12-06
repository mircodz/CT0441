#ifndef __FIELD_H__
#define __FIELD_H__

typedef struct {
  int *data;
  int  w;
  int  h;
} field_t;

field_t *field_new(int w, int h);

/**
  \brief Memory safe access to data of \p f
  */
void field_at(field_t *f, int x, int y, int v);

/**
  \brief Cleanup completed rows.
  \returns Number of cleared rows.
  */
int field_cleanup(field_t *f);

void field_shift(field_t *f, int row);

#endif /* __FIELD_H__ */
