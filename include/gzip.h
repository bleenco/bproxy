#ifndef GZIP_H
#define GZIP_H

#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "zlib.h"

typedef struct gzip_state_t
{
  z_stream strm;
  int current_size_in;
  int current_size_out;
  int current_size_buf_out;
  bool first_chunk;
  bool last_chunk;
  char *http_header;
  unsigned char *raw_body;
  unsigned char *gzip_body;
  char *chunk_body;
} gzip_state_t;

int gzip_init_state(gzip_state_t *state, char *http_header);
void gzip_free_state(gzip_state_t *state);
int gzip_compress(gzip_state_t *state);
void gzip_chunk_compress(gzip_state_t *state);

#endif
