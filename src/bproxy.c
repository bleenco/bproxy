/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/bproxy
 */
#include "bproxy.h"

void conn_init(uv_stream_t *handle)
{
  conn_t *conn = malloc(sizeof(conn_t));
  conn->parser = malloc(sizeof(http_parser));
  conn->request = malloc(sizeof(http_request_t));
  conn->proxy_handle = malloc(sizeof(conn_t));
  conn->proxy_handle = NULL;
  conn->handle = handle;
  handle->data = conn;
  http_parser_init(conn->parser, HTTP_REQUEST);
  conn->parser->data = conn;
}

void conn_free(uv_handle_t *handle)
{
  conn_t *conn = handle->data;
  if (conn)
  {
    if (conn->parser)
    {
      free(conn->parser);
    }
    if (conn->request)
    {
      free(conn->request);
    }
    if (conn)
    {
      free(conn);
    }
  }

  if (handle)
  {
    free(handle);
  }
}

void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
  *buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

void write_buf(uv_stream_t *handle, char *data, int len)
{
  write_req_t *wr;
  wr = (write_req_t *)malloc(sizeof *wr);
  wr->buf = uv_buf_init((char *)data, len);
  if (uv_is_writable((const uv_stream_t *)handle) && !uv_is_closing((const uv_handle_t *)handle) && uv_write(&wr->req, handle, &wr->buf, 1, write_cb))
  {
    fprintf(stderr, "write error: could not write to destination!\n");
    return;
  }
}

void write_cb(uv_write_t *req, int status)
{
  if (status < 0)
  {
    fprintf(stderr, "write error: error writing to destination!\n");
    return;
  }
}

void close_cb(uv_handle_t *peer)
{
  conn_free(peer);
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
    free(proxy_conn);
  }
}

void proxy_read_cb(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
{
  proxy_t *proxy_conn = handle->data;
  conn_t *conn = proxy_conn->conn;

  if (nread >= 0)
  {
    char *resp = malloc(nread + 1);
    memcpy(resp, buf->base, nread);
    resp[nread] = '\0';

    if (conn->request->enable_compression)
    {
      // enable gzip?
    }

    write_buf(conn->handle, resp, nread);
  }
  else
  {
    if (nread != UV_EOF)
    {
      fprintf(stderr, "error reading from socket!\n");
    }
    if (uv_is_closing((const uv_handle_t *)handle))
    {
      uv_close((uv_handle_t *)handle, proxy_close_cb);
    }
  }

  free(buf->base);
}

void proxy_connect_cb(uv_connect_t *req, int status)
{
  proxy_t *proxy_conn = req->handle->data;
  proxy_conn->connect_req = *req;

  if (proxy_conn->conn->ws_handshake_sent) {
    proxy_conn->conn->proxy_handle = req->handle;
  }

  uv_read_start(req->handle, alloc_cb, proxy_read_cb);
  write_buf(req->handle, proxy_conn->conn->request->raw, strlen(proxy_conn->conn->request->raw));
}

void proxy_http_request(char *ip, unsigned short port, conn_t *conn)
{
  struct sockaddr_in dest;
  uv_ip4_addr(ip, port, &dest);

  proxy_t *proxy_conn = malloc(sizeof(proxy_t));
  proxy_conn->conn = conn;
  proxy_conn->tcp.data = proxy_conn;

  if (conn->request->upgrade)
  {
    conn->ws_handshake_sent = true;
  }

  uv_tcp_init(server->loop, &proxy_conn->tcp);
  uv_tcp_keepalive(&proxy_conn->tcp, 1, 60);
  uv_tcp_connect(&proxy_conn->connect_req, &proxy_conn->tcp, (const struct sockaddr *)&dest, proxy_connect_cb);
}

void read_cb(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
{
  conn_t *conn = handle->data;

  if (nread >= 0)
  {
    if (!conn->ws_handshake_sent)
    {
      conn->request->raw = malloc(nread + 1);
      memcpy(conn->request->raw, buf->base, nread);
      conn->request->raw[nread] = '\0';

      size_t np = http_parser_execute(conn->parser, &parser_settings, buf->base, nread);
      if (np != nread)
      {
        uv_shutdown_t *req;
        req = (uv_shutdown_t *)malloc(sizeof *req);
        uv_shutdown(req, handle, shutdown_cb);
      }

      proxy_ip_port ip_port = find_proxy_config(conn->request->hostname);
      if (!ip_port.ip || !ip_port.port)
      {
        char *resp = malloc(1024 * sizeof(char));
        http_404_response(resp);
        write_buf(conn->handle, resp, strlen(resp));
        free(resp);
        return;
      }

      if (!conn->parser->upgrade)
      {
        fprintf(stderr, "[http]: %s\n", conn->request->url);
      }
      else
      {
        fprintf(stderr, "[ws]: %s\n", conn->request->url);
      }

      proxy_http_request(ip_port.ip, ip_port.port, conn);
    }
    else if (conn->proxy_handle)
    {
      write_buf(conn->proxy_handle, buf->base, nread);
    }
  }
  else
  {
    if (nread != UV_EOF)
    {
      fprintf(stderr, "error reading from socket!\n");
    }
    if (uv_is_closing((const uv_handle_t *)handle))
    {
      uv_close((uv_handle_t *)handle, close_cb);
    }
  }

  free(buf->base);
}

void connection_cb(uv_stream_t *s, int status)
{
  if (status < 0)
  {
    fprintf(stderr, "connection error: %s", uv_err_name(status));
    return;
  }

  uv_stream_t *conn;
  conn = malloc(sizeof(uv_tcp_t));

  if (uv_tcp_init(server->loop, (uv_tcp_t *)conn))
  {
    fprintf(stderr, "cannot init tcp connection!\n");
    return;
  }

  if (uv_accept(s, conn))
  {
    fprintf(stderr, "cannot accept tcp connection!\n");
    return;
  }

  conn_init(conn);
  if (uv_read_start(conn, alloc_cb, read_cb))
  {
    fprintf(stderr, "cannot read from remote connection!\n");
    return;
  }
}

proxy_ip_port find_proxy_config(char *hostname)
{
  proxy_ip_port ip_port = {.ip = NULL, .port = 0};

  for (int i = 0; i < server->config->num_proxies; i++)
  {
    proxy_config_t *pconf = server->config->proxies[i];
    for (int j = 0; j < pconf->num_hosts; j++)
    {
      if (strcmp(pconf->hosts[j], hostname) == 0)
      {
        ip_port.ip = pconf->ip;
        ip_port.port = pconf->port;
        return ip_port;
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
    fprintf(stderr, "cannot initialize tcp connection.\n");
    return 1;
  }
  uv_ip4_addr("0.0.0.0", server->config->port, &server->address);
  if (uv_tcp_bind(&server->tcp, (const struct sockaddr *)&server->address, 0))
  {
    fprintf(stderr, "cannot bind server.\n");
    return 1;
  }
  if (uv_listen((uv_stream_t *)&server->tcp, 4096, connection_cb))
  {
    fprintf(stderr, "server listen error.\n");
    return 1;
  }
  fprintf(stderr, "=> bproxy listening on 0.0.0.0:%d\n", server->config->port);

  return 0;
}

void parse_args(int argc, char **argv)
{
  server->config_file = malloc(256 * sizeof(char));
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
