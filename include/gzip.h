#ifndef GZIP_H
#define GZIP_H

#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "zlib.h"
#include "http.h"

typedef struct gzip_state_t
{
  z_stream strm;
  int current_size_in;
  int current_size_out;
  int current_size_buf_out;
  boolean first_chunk;
  boolean last_chunk;
  char *http_header;
  unsigned char *raw_body;
  unsigned char *gzip_body;
  char *chunk_body;
} gzip_state_t;

int gzip_init_state(gzip_state_t *state);
void gzip_free_state(gzip_state_t *state);
int gzip_compress(gzip_state_t *state);
void gzip_chunk_compress(gzip_state_t *state);
void gzip_init_headers(gzip_state_t *state, http_response_t *response);

#endif
