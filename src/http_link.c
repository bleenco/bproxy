#include "http_link.h"
#include "common.h"

#define CHECK(V) \
  if ((V) != 0) abort()

void http_link_init(uv_link_t *link, http_link_context_t *context,
                    config_t *config) {
  memset(context, 0, sizeof *context);
  context->server_config = config;
  context->type = TYPE_REQUEST;
  link->data = context;

  http_parser_init(&context->request.parser, HTTP_REQUEST);
  http_parser_init(&context->response.parser, HTTP_RESPONSE);

  context->request.parser.data = context;
  context->response.parser.data = context;
  context->request.complete = true;
}

void alloc_cb_override(uv_link_t *link, size_t suggested_size, uv_buf_t *buf) {
  buf->base = malloc(suggested_size);
  assert(buf->base != NULL);
  buf->len = suggested_size;
}

void http_read_cb_override(uv_link_t *link, ssize_t nread,
                           const uv_buf_t *buf) {
  http_link_context_t *context = link->data;

  if (nread > 0) {
    size_t http_headers_len = 0;
    if (context->type == TYPE_REQUEST) {
      if (context->request.complete) {
        context->request.complete = false;
        context->initial_reply = true;
        context->request_time = uv_hrtime();
        time(&context->request_timestamp);

        context->request.raw_len = nread;

        // Parse status line
        size_t status_line_len;
        free(context->request.status_line);
        char *status_line_end = strnstr_custom(buf->base, nread, "\r");
        if (status_line_end) {
          status_line_len = status_line_end - buf->base;
        } else {
          status_line_len = nread;
        }

        context->request.status_line = malloc(status_line_len + 1);
        memcpy(context->request.status_line, buf->base, status_line_len);
        context->request.status_line[status_line_len] = '\0';
        http_parser_init(&context->request.parser, HTTP_REQUEST);

        gzip_state_t *state = context->response.gzip_state;
        if (state) {
          gzip_free_state(state);
          free(state);
          context->response.gzip_state = NULL;
        }
        context->response.processed_data_len = 0;

        char *header_end = strnstr_custom(buf->base, nread, "\r\n\r\n");
        if (header_end) {
          http_headers_len = header_end - buf->base + 4;
        }
      }

      size_t np = http_parser_execute(&context->request.parser,
                                      &parser_settings, buf->base, nread);
      if (np != (size_t)nread) {
        uv_shutdown_t *req;
        req = (uv_shutdown_t *)malloc(sizeof *req);
        log_error("Http parsing failed");
        uv_link_propagate_read_cb(link, -400, buf);
        return;
      }

      if (context->request.upgrade) {
        context->type = TYPE_WEBSOCKET;
      }
    }
    // Insert head
    if (http_headers_len) {
      http_init_request_headers(context);
      uv_buf_t tmp_buf = uv_buf_init(malloc(context->request.http_header_len),
                                     context->request.http_header_len);
      memcpy(tmp_buf.base, context->request.http_header,
             context->request.http_header_len);
      uv_link_propagate_read_cb(link, context->request.http_header_len,
                                &tmp_buf);
    }
    // Insert body
    size_t body_size = nread - http_headers_len;
    if (body_size > 0) {
      uv_buf_t tmp_buf = uv_buf_init(malloc(body_size), body_size);
      memcpy(tmp_buf.base, &buf->base[http_headers_len], body_size);
      uv_link_propagate_read_cb(link, body_size, &tmp_buf);
    }
    nread = 0;
  }
  // Closing everything is observer's job, just propagate it to him
  uv_link_propagate_read_cb(link, nread, buf);
}

void compress_data(http_response_t *response, char *data, int len,
                   char **compressed_resp, size_t *compressed_resp_size) {
  response->gzip_state->raw_body = (unsigned char *)data;
  response->gzip_state->current_size_in = len;

  if (response->processed_data_len > 0) {
    response->gzip_state->first_chunk = false;
  }

  response->processed_data_len += len;
  if (response->processed_data_len == response->expected_data_len) {
    response->gzip_state->last_chunk = true;
  }

  gzip_compress(response->gzip_state);
  gzip_chunk_compress(response->gzip_state);

  free(*compressed_resp);
  (*compressed_resp_size) = response->gzip_state->current_size_out;
  (*compressed_resp) = malloc(response->gzip_state->current_size_out);
  memcpy(*compressed_resp, response->gzip_state->chunk_body,
         response->gzip_state->current_size_out);
}

int http_link_write(uv_link_t *link, uv_link_t *source, const uv_buf_t bufs[],
                    unsigned int nbufs, uv_stream_t *send_handle,
                    uv_link_write_cb cb, void *arg) {
  http_link_context_t *context = (http_link_context_t *)link->data;
  http_response_t *response = &context->response;

  char *resp = bufs[0].base;
  size_t nread = bufs[0].len;
  size_t resp_size = nread;

  if (nread > 0) {
    int header_len = 0;
    int body_len = 0;

    if (context->initial_reply) {
      context->initial_reply = false;
      // Init parser and set status line len
      http_parser_init(&response->parser, HTTP_RESPONSE);

      // Parse status line
      size_t status_line_len;
      char *status_line_end = strnstr_custom(resp, nread, "\r");
      if (status_line_end) {
        status_line_len = status_line_end - resp;
      } else {
        log_warn("HTTP_PARSING (response status line): Len: %d; %.*s", nread,
                 nread, resp);
        status_line_len = nread;
      }

      memcpy(response->status_line, resp, status_line_len);
      response->status_line[status_line_len] = '\0';

      // Log output
      double timeDiff = (uv_hrtime() - context->request_time) / 1000000.0;
      struct tm *timeinfo;
      timeinfo = localtime(&context->request_timestamp);
      char timeString[256];
      strftime(timeString, sizeof(timeString), "%d/%b/%Y:%T %z", timeinfo);
      // IP - [response time] - [date time] "GET url http" HTTP_STATUS_NUM host
      log_debug("%s - [%.3fms] - [%s] \"%s\" %u %s", context->peer_ip, timeDiff,
                timeString, context->request.status_line,
                response->parser.status_code, context->request.host);
    }
    if (!context->response.headers_received) {
      // Keep parsing until all headers have arrived
      // Because parser quits on headers received, parsed length means header
      // length. Added +1, because parser doesn't count last LF after headers
      header_len = http_parser_execute(&response->parser, &resp_parser_settings,
                                       resp, nread) +
                   1;
    }
    if (!context->response.headers_received) {
      // Headers not yet received, free response and nothing else
      free(resp);
      resp = NULL;
      return 0;
    }
    if (context->response.headers_received && !context->response.headers_send) {
      // Headers have arrived , but are not yet processed
      // Init headers response and check if body follows headers
      bool use_compression = context->type == TYPE_REQUEST &&
                             context->request.enable_compression &&
                             response->enable_compression;
      http_init_response_headers(response, use_compression);
      // Get body start and body length
      body_len = nread - header_len;
      response->body_size = body_len;
    }
    if (context->type == TYPE_REQUEST && context->request.enable_compression &&
        response->enable_compression) {
      context->response.headers_send = true;
      if (response->gzip_state == NULL) {
        response->gzip_state = malloc(sizeof *response->gzip_state);
        gzip_init_state(response->gzip_state, response->http_header);
        if (body_len == 0) {
          resp_size = 0;
        }
        if (body_len > 0) {
          free(response->raw_body);
          response->raw_body = malloc(body_len);
          memcpy(response->raw_body, &resp[header_len], body_len);

          compress_data(response, response->raw_body, response->body_size,
                        &resp, &resp_size);
        }
      } else  // Gzip initialized
      {
        compress_data(response, resp, nread, &resp, &resp_size);
      }
    } else {
      // Add Via header
      if (!context->response.headers_send) {
        context->response.headers_send = true;
        char *tmp_resp = malloc(response->http_header_len + body_len);
        memcpy(tmp_resp, response->http_header, response->http_header_len);
        memcpy(&tmp_resp[response->http_header_len], &resp[header_len],
               body_len);
        free(resp);
        resp = tmp_resp;
        resp_size = response->http_header_len + body_len;
      }
    }
  }
  uv_buf_t tmp_buf = uv_buf_init(resp, resp_size);
  return uv_link_propagate_write(link->parent, source, &tmp_buf, 1, send_handle,
                                 cb, resp);
}

void http_write_link_cb(uv_link_t *source, int status, void *arg) { free(arg); }

void http_link_close(uv_link_t *link, uv_link_t *source, uv_link_close_cb cb) {
  http_link_context_t *context = (http_link_context_t *)link->data;
  http_response_t *response = &context->response;

  gzip_free_state(response->gzip_state);
  free(response->gzip_state);
  context->request.raw_len = 0;
  free(context->response.raw_body);
  free(context->request.status_line);
  free(context->request.body);
  free(context->request.url);
  cb(source);
}

uv_link_methods_t http_link_methods = {
    .read_start = uv_link_default_read_start,
    .read_stop = uv_link_default_read_stop,
    .shutdown = uv_link_default_shutdown,
    .close = http_link_close,
    .write = http_link_write,

    /* Other doesn't matter in this example */
    .alloc_cb_override = alloc_cb_override,
    .read_cb_override = http_read_cb_override};
