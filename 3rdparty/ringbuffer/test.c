#include "ringbuffer.h"

#include <assert.h>  /* assert */
#include <stdio.h>  /* fprintf */
#include <stdlib.h>  /* malloc */
#include <string.h>  /* memcpy */

#define TEST_DATA_SIZE 256 * 1024 * 1024
#define TEST_INSERT_OFF RING_BUFFER_LEN * 2 - 8
#define TEST_INSERT_LEN 16
#define ASSERT(e) \
    if (!(e)) {\
      fprintf(stderr, "ASSERT: " #e " failed on %d\n", __LINE__); \
      abort(); \
    }

static char* data;
static ringbuffer rb;

int main() {
  int i;
  int j;
  int r;
  int after;
  ssize_t len;
  char* ptr;

  data = malloc(TEST_DATA_SIZE);
  assert(data != NULL);
  ringbuffer_init(&rb);

  /* Fill test data */
  for (i = 0; i < TEST_DATA_SIZE; i++)
    data[i] = (i * i) % 137;

  /* Fill ringbuffer */
  i = 0;
  after = 0;
  while (i < TEST_DATA_SIZE - TEST_INSERT_LEN) {
    if (after)
      len = TEST_DATA_SIZE - i - TEST_INSERT_LEN;
    else
      len = TEST_INSERT_OFF - i;

    ptr = ringbuffer_write_ptr(&rb, &len);
    ASSERT(ptr != NULL);

    /* Always make progress */
    ASSERT(len > 0);

    if (after)
      memcpy(ptr, data + i + TEST_INSERT_LEN, len);
    else
      memcpy(ptr, data + i, len);

    i += len;
    r = ringbuffer_write_append(&rb, len);
    ASSERT(r == 0);

    if (i == TEST_INSERT_OFF)
      after = 1;
  }
  ASSERT(ringbuffer_size(&rb) == TEST_DATA_SIZE - TEST_INSERT_LEN);

  /* Insert stuff */
  ringbuffer_insert(&rb,
                    TEST_INSERT_OFF,
                    data + TEST_INSERT_OFF,
                    TEST_INSERT_LEN);

  /* Read from it */
  i = 0;
  while (i < TEST_DATA_SIZE) {
    len = TEST_DATA_SIZE - i;
    ptr = ringbuffer_read_next(&rb, &len);
    ASSERT(ptr != NULL);

    /* Always make progress */
    ASSERT(len > 0);

    for (j = 0; j < len; j++)
      ASSERT(ptr[j] == data[i + j]);

    ringbuffer_read_skip(&rb, len);
    i += len;
  }

  /* Destroy it */
  ringbuffer_destroy(&rb);

  return 0;
}
