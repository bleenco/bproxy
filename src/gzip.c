#include "gzip.h"

#include "zlib.h"
#include <stdio.h>
#include <string.h>
#include "stdint.h"

int gzip_init_state(gzip_state_t *state)
{
  int ret_status = 0;

  memset(state, 0, sizeof *state);

  state->current_size_in = 0;
  state->current_size_out = 0;
  state->first_chunk = true;
  state->last_chunk = false;

  /* allocate deflate state */
  state->strm.zalloc = Z_NULL;
  state->strm.zfree = Z_NULL;
  state->strm.opaque = Z_NULL;

  size_t buf_sizes = 6000000;
  state->raw_body = NULL;
  state->gzip_body = malloc(buf_sizes);
  state->chunk_body = malloc(buf_sizes);
  state->http_header = malloc(buf_sizes);

  memset(state->http_header, 0, buf_sizes);

  state->current_size_buf_out = buf_sizes;

  ret_status = deflateInit2(&state->strm, -1, 8, 15 + 16, 8, 0);
  if (ret_status != Z_OK)
  {
    printf("Init failed!");
  }
  return ret_status;
}

void gzip_free_state(gzip_state_t *state)
{
  if (state)
  {
    deflateEnd(&state->strm);
    free(state->gzip_body);
    free(state->chunk_body);
    free(state->http_header);
  }
}

int gzip_compress(gzip_state_t *state)
{
  state->strm.next_in = state->raw_body;
  state->strm.next_out = state->gzip_body;

  state->strm.avail_in = state->current_size_in;
  state->strm.avail_out = state->current_size_buf_out;

  (void)deflate(&state->strm, Z_BLOCK);
  if (state->last_chunk)
  {
    (void)deflate(&state->strm, Z_FINISH);
  }

  size_t gz_len = state->current_size_buf_out - state->strm.avail_out;

  state->current_size_out = gz_len;

  return 0;
}

void gzip_chunk_compress(gzip_state_t *state)
{
  int gz_size = state->current_size_out;
  // Message def: hex size, \r\n, gzip content, \r\n, (if last add) 0 \r\n\r\n
  char *head_def = "%X\r\n";

  char head[200];

  char *tail = "\r\n";
  if (state->last_chunk)
  {
    tail = "\r\n0\r\n\r\n";
  }

  int head_len = sprintf(head, head_def, gz_size);
  int tail_len = strlen(tail);

  size_t offset = 0;
  size_t copy_len;

  if (state->first_chunk)
  {
    // Copy http header
    copy_len = strlen(state->http_header);
    memcpy(state->chunk_body + offset, state->http_header, copy_len);
    offset += copy_len;
  }

  // Copy head
  copy_len = head_len;
  memcpy(state->chunk_body + offset, head, copy_len);
  offset += copy_len;

  // Copy body
  copy_len = gz_size;
  memcpy(state->chunk_body + offset, state->gzip_body, copy_len);
  offset += copy_len;

  // Copy tail
  copy_len = tail_len;
  memcpy(state->chunk_body + offset, tail, copy_len);
  offset += copy_len;

  state->current_size_out = offset;
}
