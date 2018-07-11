#include "src/link_methods.h"
#include "src/common.h"
#include "src/errors.h"
#include "src/private.h"
#include "src/queue.h"

static int uv_ssl_link_read_start(uv_link_t* link);
static int uv_ssl_link_read_stop(uv_link_t* link);
static int uv_ssl_link_write(uv_link_t* link, uv_link_t* source,
                             const uv_buf_t bufs[], unsigned int nbufs,
                             uv_stream_t* send_handle, uv_link_write_cb cb,
                             void* arg);
static int uv_ssl_link_try_write(uv_link_t* link, const uv_buf_t bufs[],
                                 unsigned int nbufs);
static int uv_ssl_link_shutdown(uv_link_t* link, uv_link_t* source,
                                uv_link_shutdown_cb cb, void* arg);
static void uv_ssl_link_close(uv_link_t* link, uv_link_t* source,
                              uv_link_close_cb cb);
static const char* uv_ssl_link_strerror(uv_link_t* link, int err);
static void uv_ssl_alloc_cb_override(uv_link_t* link,
                                     size_t suggested_size,
                                     uv_buf_t* buf);
static void uv_ssl_read_cb_override(uv_link_t* link,
                                    ssize_t nread,
                                    const uv_buf_t* buf);

uv_link_methods_t uv_ssl_methods = {
  .read_start = uv_ssl_link_read_start,
  .read_stop = uv_ssl_link_read_stop,

  .write = uv_ssl_link_write,
  .try_write = uv_ssl_link_try_write,

  .shutdown = uv_ssl_link_shutdown,
  .close = uv_ssl_link_close,
  .strerror = uv_ssl_link_strerror,

  .alloc_cb_override = uv_ssl_alloc_cb_override,
  .read_cb_override = uv_ssl_read_cb_override
};


int uv_ssl_link_read_start(uv_link_t* link) {
  uv_ssl_t* ssl;
  int internal;
  int err;

  ssl = (uv_ssl_t*) link;

  err = uv_ssl_pop_error(ssl);
  if (err != 0)
    return err;

  err = uv_ssl_cycle(ssl);
  if (err != 0)
    return err;

  internal = ssl->state == kSSLStateHandshake;
  ssl->state = kSSLStateData;

  /* Already state, skip calling parent */
  if (internal)
    return 0;

  return uv_link_read_start(link->parent);
}


int uv_ssl_link_read_stop(uv_link_t* link) {
  uv_ssl_t* ssl;
  int err;

  ssl = (uv_ssl_t*) link;

  err = uv_ssl_pop_error(ssl);
  if (err != 0)
    return err;

  ssl->state = kSSLStateNone;

  return uv_link_read_stop(link->parent);
}


int uv_ssl_link_write(uv_link_t* link, uv_link_t* source, const uv_buf_t bufs[],
                      unsigned int nbufs, uv_stream_t* send_handle,
                      uv_link_write_cb cb, void* arg) {
  uv_ssl_t* ssl;

  /* No IPC writes on uv_ssl_t */
  if (send_handle != NULL)
    return UV_ENOSYS;

  ssl = (uv_ssl_t*) link;

  return uv_ssl_write(ssl, source, bufs, nbufs, cb, arg);
}


int uv_ssl_link_try_write(uv_link_t* link, const uv_buf_t bufs[],
                          unsigned int nbufs) {
  uv_ssl_t* ssl;

  ssl = (uv_ssl_t*) link;

  return uv_ssl_sync_write(ssl, bufs, nbufs);
}


int uv_ssl_link_shutdown(uv_link_t* link, uv_link_t* source,
                         uv_link_shutdown_cb cb, void* arg) {
  uv_ssl_t* ssl;

  ssl = (uv_ssl_t*) link;

  return uv_ssl_shutdown(ssl, source, cb, arg);
}


void uv_ssl_link_close(uv_link_t* link, uv_link_t* source,
                       uv_link_close_cb cb) {
  uv_ssl_t* ssl;

  ssl = (uv_ssl_t*) link;
  uv_ssl_destroy(ssl, source, cb);
}


const char* uv_ssl_link_strerror(uv_link_t* link, int err) {
  switch (err) {
    case kUVSSLErrUnexpectedEOF:
      return "uv_ssl_t: unexpected UV_EOF";
    case kUVSSLErrCycleInput:
      return "uv_ssl_t: SSL_read() error";
    case kUVSSLErrSSLWrite:
    case kUVSSLErrSSLSyncWrite:
      return "uv_ssl_t: SSL_write() error";
    default:
      return NULL;
  }
}


void uv_ssl_alloc_cb_override(uv_link_t* link,
                              size_t suggested_size,
                              uv_buf_t* buf) {
  uv_ssl_t* ssl;
  size_t avail;
  char* ptr;

  ssl = (uv_ssl_t*) link;

  avail = 0;
  ptr = ringbuffer_write_ptr(&ssl->encrypted.input, &avail);
  *buf = uv_buf_init(ptr, avail);
}


void uv_ssl_read_cb_override(uv_link_t* link,
                             ssize_t nread,
                             const uv_buf_t* buf) {
  int r;
  uv_ssl_t* ssl;

  ssl = (uv_ssl_t*) link;

  if(nread > 0 && !ssl->initial_buf.len){
    char *buf_base = malloc(nread);
    memcpy(buf_base, buf->base, nread);
    ssl->initial_buf = uv_buf_init(buf_base, nread);
  }

  /* Commit data if there was no error */
  r = 0;
  if (nread >= 0) {
    r = ringbuffer_write_append(&ssl->encrypted.input, nread);
    if (r != 0)
      return uv_ssl_error(ssl, UV_ENOMEM);
  }

  /* Handle EOF */
  if (nread == UV_EOF)
    return uv_ssl_error(ssl, kUVSSLErrUnexpectedEOF);
  else if (nread < 0)
    return uv_ssl_error(ssl, nread);


  SSL_CTX_set_tlsext_servername_arg(SSL_get_SSL_CTX(ssl->ssl), ssl->data);
  r = uv_ssl_cycle(ssl);

  if (r != 0)
    uv_ssl_error(ssl, r);
}
