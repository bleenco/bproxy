#include "http_link.h"

#define CHECK(V) \
  if ((V) != 0)  \
  abort()

void http_link_init(uv_link_t *link, http_link_context_t *context, config_t *config)
{
  memset(context, 0, sizeof *context);
  context->server_config = config;
  context->type = TYPE_REQUEST;
  link->data = context;

  http_parser_init(&context->request.parser, HTTP_REQUEST);
  http_parser_init(&context->response.parser, HTTP_RESPONSE);

  context->request.parser.data = context;
  context->response.parser.data = context;
}

void alloc_cb_override(uv_link_t *link,
                       size_t suggested_size,
                       uv_buf_t *buf)
{
  buf->base = malloc(suggested_size);
  assert(buf->base != NULL);
  buf->len = suggested_size;
}

void http_read_cb_override(uv_link_t *link, ssize_t nread, const uv_buf_t *buf)
{
  http_link_context_t *context = link->data;

  if (nread >= 0)
  {
    if (context->type == TYPE_REQUEST)
    {
      free(context->request.raw);
      context->request.raw = malloc(nread);
      memcpy(context->request.raw, buf->base, nread);
      context->request.raw_len = nread;

      size_t np = http_parser_execute(&context->request.parser, &parser_settings, buf->base,
                                      nread);

      if (context->request.upgrade)
      {
        context->type = TYPE_WEBSOCKET;
      }
      if (np != nread)
      {
        uv_shutdown_t *req;
        req = (uv_shutdown_t *)malloc(sizeof *req);
        log_error("Http parsing failed");
        // TODO: send back error 400 Bad Request in observer
        uv_link_propagate_read_cb(link, -400, buf);
        return;
      }

      gzip_state_t *state = context->response.gzip_state;
      if (state)
      {
        gzip_free_state(state);
        free(state);
        context->response.gzip_state = NULL;
      }
      context->response.processed_data_len = 0;

      if (!context->request.parser.upgrade)
      {
        log_debug("[http]: %s", context->request.url);
      }
      else
      {
        log_debug("[websocket]: %s", context->request.url);
      }
    }
  }
  // Closing everything is observer's job, just propagate it to him
  uv_link_propagate_read_cb(link, nread, buf);
}

void compress_data(http_response_t *response, char *data, int len, char **compressed_resp, size_t *compressed_resp_size)
{
  response->gzip_state->raw_body = (unsigned char *)data;
  response->gzip_state->current_size_in = len;

  if (response->processed_data_len > 0)
  {
    response->gzip_state->first_chunk = false;
  }

  response->processed_data_len += len;
  if (response->processed_data_len == response->expected_data_len)
  {
    response->gzip_state->last_chunk = true;
  }

  gzip_compress(response->gzip_state);
  gzip_chunk_compress(response->gzip_state);

  free(*compressed_resp);
  (*compressed_resp_size) = response->gzip_state->current_size_out;
  (*compressed_resp) = malloc(response->gzip_state->current_size_out);
  memcpy(*compressed_resp, response->gzip_state->chunk_body, response->gzip_state->current_size_out);
}

int http_link_write(uv_link_t *link, uv_link_t *source, const uv_buf_t bufs[], unsigned int nbufs, uv_stream_t *send_handle, uv_link_write_cb cb, void *arg)
{
  http_link_context_t *context = (http_link_context_t *)link->data;
  http_response_t *response = &context->response;

  char *resp = bufs[0].base;
  size_t nread = bufs[0].len;
  size_t resp_size = nread;

  if (context->type == TYPE_REQUEST && context->request.enable_compression)
  {
    // enable gzip?
    if (response->gzip_state == NULL)
    {
      http_parser_init(&response->parser, HTTP_RESPONSE);
      http_parser_execute(&response->parser, &resp_parser_settings, resp, nread);
      if (response->enable_compression)
      {
        // Parse status line
        int status_line_len = strchr(resp, '\r') - resp;
        memcpy(response->status_line, resp, status_line_len);
        response->status_line[status_line_len] = '\0';

        // Get body start and body length
        int headers_len = strstr(resp, "\r\n\r\n") - resp + 4; // 4 is for \r\n\r\n
        int body_len = nread - headers_len;
        response->body_size = body_len;

        response->gzip_state = malloc(sizeof *response->gzip_state);
        gzip_init_state(response->gzip_state);
        gzip_init_headers(response);
        if (body_len == 0)
        {
          resp_size = 0;
        }
        if (body_len > 0)
        {
          free(response->raw_body);
          response->raw_body = malloc(body_len);
          memcpy(response->raw_body, &resp[headers_len], body_len);

          compress_data(response, response->raw_body,
                        response->body_size, &resp, &resp_size);
        }
      }
      else
      {
        context->request.enable_compression = false;
      }
    }
    else // Gzip initialized
    {
      compress_data(response, resp, nread, &resp, &resp_size);
    }
  }
  uv_buf_t tmp_buf = uv_buf_init(resp, resp_size);
  return uv_link_propagate_write(link->parent, source, &tmp_buf, 1, send_handle, cb, resp);
}

void http_write_link_cb(uv_link_t *source, int status, void *arg)
{
  free(arg);
}

void http_link_close(uv_link_t *link, uv_link_t *source, uv_link_close_cb cb)
{
  http_link_context_t *context = (http_link_context_t *)link->data;
  http_response_t *response = &context->response;

  gzip_free_state(response->gzip_state);
  free(response->gzip_state);
  free(context->request.raw);
  context->request.raw_len = 0;
  free(context->response.raw_body);
  free(context->request.body);
  free(context->request.url);
  cb(source);
}

uv_link_methods_t http_link_methods = {
    .read_start = uv_link_default_read_start,
    .read_stop = uv_link_default_read_stop,
    .close = http_link_close,
    .write = http_link_write,

    /* Other doesn't matter in this example */
    .alloc_cb_override = alloc_cb_override,
    .read_cb_override = http_read_cb_override};
