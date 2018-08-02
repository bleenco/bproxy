#ifndef HTTP_LINK_H
#define HTTP_LINK_H

#include <assert.h>
#include "config.h"
#include "gzip.h"
#include "http.h"
#include "log.h"
#include "uv_link_t.h"

void http_link_init(uv_link_t *link, http_link_context_t *context,
                    config_t *config);
void http_write_link_cb(uv_link_t *source, int status, void *arg);

static void alloc_cb_override(uv_link_t *link, size_t suggested_size,
                              uv_buf_t *buf);
static void http_read_cb_override(uv_link_t *link, ssize_t nread,
                                  const uv_buf_t *buf);
static void compress_data(http_response_t *response, char *data, int len,
                          char **compressed_resp, size_t *compressed_resp_size);
static int http_link_write(uv_link_t *link, uv_link_t *source,
                           const uv_buf_t bufs[], unsigned int nbufs,
                           uv_stream_t *send_handle, uv_link_write_cb cb,
                           void *arg);
void http_free_raw_requests_queue(http_request_t *request);
uv_link_methods_t http_link_methods;

#endif
