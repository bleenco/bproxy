/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/bproxy
 */
#include "bproxy.h"
#include "gzip.h"
#include "log.h"

#include <assert.h>

#define CHECK(V) if ((V) != 0) abort()


static void alloc_cb_override(uv_link_t* link,
                              size_t suggested_size,
                              uv_buf_t* buf) {
  buf->base = malloc(suggested_size);
  assert(buf->base != NULL);
  buf->len = suggested_size;
}

static void write_link_cb(uv_link_t* source, int status, void* arg)
{
  free(arg);
}

static void observer_connection_link_close_cb(uv_link_t* link) {
  conn_t* conn;

  conn = link->data;
  conn_free(conn);
}

static void observer_connection_read_cb(uv_link_observer_t* observer, ssize_t nread,
                                   const uv_buf_t* buf) {
  if (nread >= 0)
    return;

  uv_link_close((uv_link_t*) observer, observer_connection_link_close_cb);
}

void conn_init(uv_stream_t *handle)
{
  conn_t *conn = malloc(sizeof(conn_t));
  conn->parser = malloc(sizeof(http_parser));
  conn->request = malloc(sizeof(http_request_t));
  memset(conn->request, 0, sizeof *conn->request);
  conn->proxy_handle = NULL;
  conn->handle = handle;
  http_parser_init(conn->parser, HTTP_REQUEST);
  conn->parser->data = conn;
  conn->ws_handshake_sent = false;
  conn->type = TYPE_REQUEST;

  CHECK(uv_link_source_init(&conn->source, (uv_stream_t*) conn->handle));
  conn->source.data = conn;
  CHECK(uv_link_init(&conn->middle, &middle_methods));
  CHECK(uv_link_observer_init(&conn->observer));
  conn->middle.data = conn;
  CHECK(uv_link_chain((uv_link_t*)&conn->source, &conn->middle));
  CHECK(uv_link_chain((uv_link_t*)&conn->middle, (uv_link_t*)&conn->observer));

  conn->observer.observer_read_cb = observer_connection_read_cb;
  conn->observer.data = conn;
  CHECK(uv_link_read_start((uv_link_t*) &conn->observer));
}

void conn_free(conn_t *conn)
{
  if (conn->proxy_handle)
  {
    if (!uv_is_closing((uv_handle_t *)conn->proxy_handle))
    {
      uv_close((uv_handle_t *)conn->proxy_handle, proxy_close_cb);
    }
  }
  if (conn)
  {
    if (conn->parser)
    {
      free(conn->parser);
    }
    if (conn->request)
    {
      free(conn->request->url);
      free(conn->request->raw);
      free(conn->request);
    }
  }
  free(conn->handle);
  free(conn);
}

void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
  *buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

void write_buf(uv_stream_t *handle, char *data, int len)
{
  if (len == 0)
  {
    return;
  }
  write_req_t *wr;
  wr = (write_req_t *)malloc(sizeof *wr);
  char *d = malloc(len);
  memcpy(d, data, len);
  wr->buf = uv_buf_init((char *)d, len);
  wr->req.data = wr;
  if (uv_is_writable((const uv_stream_t *)handle) && !uv_is_closing((const uv_handle_t *)handle) && uv_write(&wr->req, handle, &wr->buf, 1, write_cb))
  {
    log_error("could not write to destination!");
    return;
  }
}

void write_cb(uv_write_t *req, int status)
{
  if (status < 0)
  {
    log_error("error writting to destination!");
    return;
  }
  write_req_t *wr = (write_req_t *)req->data;
  free(wr->buf.base);
  free(wr);
}

void close_cb(uv_handle_t *peer)
{
  conn_t* conn = (conn_t*)(((uv_link_source_t*)peer->data)->data);
  conn_free(conn);
}

void link_close_cb(uv_link_t* source)
{
  conn_free((conn_t *)source->data);
}

void shutdown_cb(uv_shutdown_t *req, int status)
{
  uv_close((uv_handle_t *)req->handle, close_cb);
  free(req);
}

void proxy_close_cb(uv_handle_t *peer)
{
  proxy_t *proxy_conn = peer->data;
  if (proxy_conn)
  {
    if (proxy_conn->gzip_state)
    {
      gzip_free_state(proxy_conn->gzip_state);
      free(proxy_conn->gzip_state);
    }
    free(proxy_conn->response.raw_body);
    free(proxy_conn);
  }
}

void compress_data(proxy_t *proxy_conn, char *data, int len)
{
  proxy_conn->gzip_state->raw_body = (unsigned char *)data;
  proxy_conn->gzip_state->current_size_in = len;

  if (proxy_conn->response.processed_data_len > 0)
  {
    proxy_conn->gzip_state->first_chunk = false;
  }

  proxy_conn->response.processed_data_len += len;
  if (proxy_conn->response.processed_data_len == proxy_conn->response.expected_data_len)
  {
    proxy_conn->gzip_state->last_chunk = true;
  }

  gzip_compress(proxy_conn->gzip_state);
  gzip_chunk_compress(proxy_conn->gzip_state);
}

void proxy_read_cb(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
{
  proxy_t *proxy_conn = handle->data;
  conn_t *conn = proxy_conn->conn;

  if (nread > 0)
  {
    char *resp = buf->base;
    ssize_t resp_size = nread;
    if (conn->request->enable_compression && conn->type == TYPE_REQUEST)
    {
      // enable gzip?
      if (proxy_conn->gzip_state == NULL)
      {
        http_parser_init(&proxy_conn->response.parser, HTTP_RESPONSE);
        http_parser_execute(&proxy_conn->response.parser,
                            &resp_parser_settings, resp, nread);
        if (proxy_conn->response.enable_compression)
        {
          // Parse status line
          int status_line_len = strchr(resp, '\r') - resp;
          memcpy(proxy_conn->response.status_line, resp, status_line_len);

          // Get body start and body length
          int headers_len = strstr(resp, "\r\n\r\n") - resp + 4; // 4 is for \r\n\r\n
          int body_len = nread - headers_len;
          proxy_conn->response.body_size = body_len;

          proxy_conn->gzip_state = malloc(sizeof *proxy_conn->gzip_state);
          gzip_init_state(proxy_conn->gzip_state);
          gzip_init_headers(proxy_conn->gzip_state, &proxy_conn->response);
          if (body_len == 0)
          {
            resp_size = 0;
          }
          if (body_len > 0)
          {
            proxy_conn->response.raw_body = malloc(body_len);
            memcpy(proxy_conn->response.raw_body, &resp[headers_len], body_len);

            compress_data(proxy_conn, proxy_conn->response.raw_body,
                          proxy_conn->response.body_size);
            resp = proxy_conn->gzip_state->chunk_body;
            resp_size = proxy_conn->gzip_state->current_size_out;
          }
        }
        else
        {
          conn->request->enable_compression = false;
        }
      }
      else // Gzip initialized
      {
        compress_data(proxy_conn, resp, nread);
        resp = proxy_conn->gzip_state->chunk_body;
        resp_size = proxy_conn->gzip_state->current_size_out;
      }
    }

    char* tmp_shit = malloc(resp_size);
    memcpy(tmp_shit, resp, resp_size);
    uv_buf_t tmp_buf = uv_buf_init(tmp_shit, resp_size);
    uv_link_write(&conn->middle, &tmp_buf, 1, NULL, write_link_cb, tmp_buf.base);
  }
  else if (nread < 0)
  {
    if (nread != UV_EOF)
    {
      log_error("could not read from socket!");
    }
    if (!uv_is_closing((const uv_handle_t *)handle))
    {
      uv_close((uv_handle_t *)handle, proxy_close_cb);
      conn->proxy_handle = NULL;
    }
  }

  free(buf->base);
}

void proxy_connect_cb(uv_connect_t *req, int status)
{
  proxy_t *proxy_conn = req->handle->data;
  proxy_conn->connect_req = *req;

  if (status < 0)
  {
    uv_link_close((uv_link_t*) &proxy_conn->conn->observer, observer_connection_link_close_cb);
    // TODO: write meaningful responses
    return;
  }
  proxy_conn->conn->proxy_handle = req->handle;

  http_parser_init(&proxy_conn->response.parser, HTTP_RESPONSE);
  proxy_conn->response.server_config = server->config;
  proxy_conn->response.parser.data = &proxy_conn->response;

  uv_read_start(req->handle, alloc_cb, proxy_read_cb);
  write_buf(req->handle, proxy_conn->conn->request->raw,
            strlen(proxy_conn->conn->request->raw));
}

void proxy_http_request(char *ip, unsigned short port, conn_t *conn)
{
  struct sockaddr_in dest;
  uv_ip4_addr(ip, port, &dest);

  proxy_t *proxy_conn = malloc(sizeof(proxy_t));
  memset(proxy_conn, 0, sizeof *proxy_conn);
  proxy_conn->conn = conn;
  proxy_conn->tcp.data = proxy_conn;
  proxy_conn->gzip_state = NULL;
  proxy_conn->response.processed_data_len = 0;
  if (conn->request->upgrade)
  {
    conn->ws_handshake_sent = true;
    conn->type = TYPE_WEBSOCKET;
  }

  uv_tcp_init(server->loop, &proxy_conn->tcp);
  uv_tcp_keepalive(&proxy_conn->tcp, 1, 60);
  uv_tcp_connect(&proxy_conn->connect_req, &proxy_conn->tcp,
                 (const struct sockaddr *)&dest, proxy_connect_cb);
}

static void link_shutdown_cb(uv_link_t* source, int status, void* arg)
{
  uv_link_close(source, link_close_cb);
}

static void read_cb_override(uv_link_t* link, ssize_t nread, const uv_buf_t* buf) {
  conn_t *conn = link->data;

    if (nread >= 0)
    {
      if (conn->proxy_handle && conn->type == TYPE_WEBSOCKET)
      {
        write_buf(conn->proxy_handle, buf->base, nread);
      }
      else
      {
        free(conn->request->raw);
        conn->request->raw = malloc(nread + 1);
        memcpy(conn->request->raw, buf->base, nread);
        conn->request->raw[nread] = '\0';

        size_t np = http_parser_execute(conn->parser, &parser_settings, buf->base,
                                        nread);

        if (np != nread)
        {
          uv_shutdown_t *req;
          req = (uv_shutdown_t *)malloc(sizeof *req);
          uv_link_shutdown(link, link_shutdown_cb, NULL);
          //uv_shutdown(req, handle, shutdown_cb);
        }

        if (conn->proxy_handle)
        {
          gzip_state_t *state = ((proxy_t *)conn->proxy_handle->data)->gzip_state;
          if (state)
          {
            gzip_free_state(state);
            free(state);
            ((proxy_t *)conn->proxy_handle->data)->gzip_state = NULL;
          }
          ((proxy_t *)conn->proxy_handle->data)->response.processed_data_len = 0;
          write_buf(conn->proxy_handle, conn->request->raw, nread);
        }

        if (!conn->parser->upgrade)
        {
          log_debug("[http]: %s", conn->request->url);
        }
        else
        {
          log_debug("[websocket]: %s", conn->request->url);
        }

        if (conn->proxy_handle == NULL)
        {
          proxy_ip_port ip_port = find_proxy_config(conn->request->hostname);
          if (!ip_port.ip || !ip_port.port)
          {
            char *resp = malloc(1024 * sizeof(char));
            http_404_response(resp);
            //write_buf(conn->handle, resp, strlen(resp));
            uv_buf_t tmp_buf = uv_buf_init(resp, strlen(resp));
            uv_link_write(link, &tmp_buf, 1, NULL, write_link_cb, tmp_buf.base);
            free(resp);
            return;
          }
          proxy_http_request(ip_port.ip, ip_port.port, conn);
        }
      }
    }
    else
    {
      if (nread != UV_EOF)
      {
        log_error("could not read from socket!");
      }
      if (conn->proxy_handle && !uv_is_closing((uv_handle_t *)conn->proxy_handle))
      {
        uv_close((uv_handle_t *)conn->proxy_handle, proxy_close_cb);
        conn->proxy_handle = NULL;
      };
      uv_link_close((uv_link_t*)&conn->observer, observer_connection_link_close_cb);
    }
    uv_link_propagate_read_cb(link, nread, buf);
}

void connection_cb(uv_stream_t *s, int status)
{
  if (status < 0)
  {
    log_error("connection error: %s", uv_err_name(status));
    return;
  }

  uv_stream_t *conn;
  conn = malloc(sizeof(uv_tcp_t));

  if (uv_tcp_init(server->loop, (uv_tcp_t *)conn))
  {
    log_error("cannot init tcp connection!");
    return;
  }

  if (uv_accept(s, conn))
  {
    log_error("cannot accept tcp connection!");
    return;
  }

  conn_init(conn);
  /*if (uv_read_start(conn, alloc_cb, read_cb))
  {
    log_error("cannot read from remote connection!");
    return;
  }*/
}

proxy_ip_port find_proxy_config(char *hostname)
{
  proxy_ip_port ip_port = {.ip = NULL, .port = 0};

  for (int i = 0; i < server->config->num_proxies; i++)
  {
    proxy_config_t *pconf = server->config->proxies[i];
    for (int j = 0; j < pconf->num_hosts; j++)
    {
      // Compare hostnames (taking into account asterisk)
      char *conf_host = pconf->hosts[j];
      boolean wildcard = conf_host[0] == '*';
      if (wildcard)
      {
        // Skip asterisk
        conf_host++;
      }

      ssize_t conf_i = strlen(conf_host) - 1;
      ssize_t i = strlen(hostname) - 1;

      while ((i >= 0 && conf_i >= 0) && hostname[i] == conf_host[conf_i])
      {
        --i;
        --conf_i;
      }
      if (conf_i < 0)
      {
        // If wildcard, requested hostname may be longer
        if (wildcard || i < 0)
        {
          ip_port.ip = pconf->ip;
          ip_port.port = pconf->port;
          return ip_port;
        }
      }
    }
  }

  return ip_port;
}

int server_init()
{
  server->loop = uv_default_loop();

  if (uv_tcp_init(server->loop, &server->tcp))
  {
    log_error("cannot init tcp connection!");
    return 1;
  }
  uv_ip4_addr("0.0.0.0", server->config->port, &server->address);
  if (uv_tcp_bind(&server->tcp, (const struct sockaddr *)&server->address, 0))
  {
    log_error("cannot bind server! check your permissions and another service running on same port.");
    return 1;
  }
  if (uv_listen((uv_stream_t *)&server->tcp, 4096, connection_cb))
  {
    log_error("server listen error!");
    return 1;
  }
  log_info("listening on 0.0.0.0:%d", server->config->port);

  return 0;
}

void parse_args(int argc, char **argv)
{
  server->config_file = malloc(256 * sizeof(char));
  memset(server->config_file, 0, 256);
  strncpy(server->config_file, "bproxy.json", 11);
  int opt;

  while ((opt = getopt(argc, argv, "c:h")) != -1)
  {
    switch (opt)
    {
    case 'c':
      if (1 != sscanf(optarg, "%s", server->config_file))
      {
        usage();
      }
      break;
    case 'h':
      usage();
      break;
    default:
      usage();
    }
  }

  if (!strcmp(server->config_file, ""))
  {
    usage();
  }
  else
  {
    char *json = read_file(server->config_file);
    parse_config(json, server->config);
  }
}

void usage()
{
  printf("bproxy v%s\n\n"
         "Usage: bproxy [-c <port> [-h] -d <domain>]\n"
         "\n"
         "Options:\n"
         "\n"
         " -h                Show this help message.\n"
         " -c <path>         Path to config JSON file. Default: bproxy.json\n"
         "\n"
         "Copyright (c) 2018 Bleenco GmbH https://bleenco.com\n",
         VERSION);
  exit(1);
}

int main(int argc, char **argv)
{
  server = malloc(sizeof(server_t));
  server->config = malloc(sizeof(config_t));
  parse_args(argc, argv);

  if (server_init())
  {
    return 1;
  }
  uv_run(server->loop, UV_RUN_DEFAULT);
  return 0;
}


uv_link_methods_t middle_methods = {
  .read_start = uv_link_default_read_start,
  .read_stop = uv_link_default_read_stop,
  .close = uv_link_default_close,
  .write = uv_link_default_write,

  /* Other doesn't matter in this example */
  .alloc_cb_override = alloc_cb_override,
  .read_cb_override = read_cb_override
};
