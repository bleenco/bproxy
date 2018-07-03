#include <stdlib.h>

#include "openssl/bio.h"
#include "ringbuffer.h"

#include "src/bio.h"
#include "src/common.h"

static int uv_ssl_bio_init(BIO* bio);
static int uv_ssl_bio_destroy(BIO* bio);
static int uv_ssl_bio_write(BIO* bio, const char* data, int len);
static int uv_ssl_bio_read(BIO* bio, char* out, int len);
static long uv_ssl_bio_ctrl(BIO* bio, int cmd, long num, void* ptr);


static const BIO_METHOD method = {
  BIO_TYPE_MEM,
  "uv_ssl SSL BIO",
  uv_ssl_bio_write,
  uv_ssl_bio_read,
  NULL,
  NULL,
  uv_ssl_bio_ctrl,
  uv_ssl_bio_init,
  uv_ssl_bio_destroy,
  NULL
};


BIO* uv_ssl_bio_new(ringbuffer* buffer) {
  BIO* bio = BIO_new((BIO_METHOD*) &method);
  if (bio == NULL)
    return NULL;

  bio->ptr = buffer;

  return bio;
}


int uv_ssl_bio_init(BIO* bio) {
  bio->shutdown = 1;
  bio->init = 1;
  bio->num = -1;

  return 1;
}


int uv_ssl_bio_destroy(BIO* bio) {
  bio->ptr = NULL;

  return 1;
}


int uv_ssl_bio_write(BIO* bio, const char* data, int len) {
  ringbuffer* buffer;

  BIO_clear_retry_flags(bio);

  buffer = bio->ptr;

  if (ringbuffer_write_into(buffer, data, len) == 0)
    return len;

  return -1;
}


int uv_ssl_bio_read(BIO* bio, char* out, int len) {
  int r;
  ringbuffer* buffer;

  BIO_clear_retry_flags(bio);

  buffer = bio->ptr;

  r = (int) ringbuffer_read_into(buffer, out, len);

  if (r == 0) {
    r = bio->num;
    if (r != 0)
      BIO_set_retry_read(bio);
  }

  return r;
}


long uv_ssl_bio_ctrl(BIO* bio, int cmd, long num, void* ptr) {
  ringbuffer* buffer;
  long ret;

  buffer = bio->ptr;
  ret = 1;

  switch (cmd) {
    case BIO_CTRL_EOF:
      ret = ringbuffer_is_empty(buffer);
      break;
    case BIO_C_SET_BUF_MEM_EOF_RETURN:
      bio->num = num;
      break;
    case BIO_CTRL_INFO:
      ret = (long) ringbuffer_size(buffer);
      if (ptr != NULL)
        *(void**)(ptr) = NULL;
      break;
    case BIO_CTRL_RESET:
      CHECK(0, "BIO_CTRL_RESET Unsupported");
      break;
    case BIO_C_SET_BUF_MEM:
      CHECK(0, "BIO_C_SET_BUF_MEM Unsupported");
      break;
    case BIO_C_GET_BUF_MEM_PTR:
      CHECK(0, "BIO_C_GET_BUF_MEM Unsupported");
      break;
    case BIO_CTRL_GET_CLOSE:
      ret = bio->shutdown;
      break;
    case BIO_CTRL_SET_CLOSE:
      bio->shutdown = num;
      break;
    case BIO_CTRL_WPENDING:
      ret = 0;
      break;
    case BIO_CTRL_PENDING:
      ret = (long) ringbuffer_size(buffer);
      break;
    case BIO_CTRL_DUP:
    case BIO_CTRL_FLUSH:
      ret = 1;
      break;
    case BIO_CTRL_PUSH:
    case BIO_CTRL_POP:
    default:
      ret = 0;
      break;
  }
  return ret;
}
