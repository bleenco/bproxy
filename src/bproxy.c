/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/bproxy
 */
#include "bproxy.h"
#include "log.h"

#include <assert.h>

#define CHECK(V) \
  if ((V) != 0)  \
  abort()

static void write_link_cb(uv_link_t *source, int status, void *arg)
{
  free(arg);
}

static void observer_connection_link_close_cb(uv_link_t *link)
{
  conn_t *conn;

  conn = link->data;
  conn_free(conn);
}

static void observer_connection_read_cb(uv_link_observer_t *observer, ssize_t nread,
                                        const uv_buf_t *buf)
{
  conn_t *conn = (conn_t *)observer->data;
  if (nread > 0)
  {
    if (conn->proxy_handle)
    {
      write_buf((uv_stream_t *)conn->proxy_handle, buf->base, nread);
    }
    else
    {
      proxy_ip_port ip_port = find_proxy_config(conn->http_link_context.request.hostname);
      if (!ip_port.ip || !ip_port.port)
      {
        char *resp = malloc(1024 * sizeof(char));
        http_404_response(resp);
        uv_buf_t tmp_buf = uv_buf_init(resp, strlen(resp));
        uv_link_write((uv_link_t *)observer, &tmp_buf, 1, NULL, write_link_cb, resp);
        return;
      }
      proxy_http_request(ip_port.ip, ip_port.port, conn);
    }
  }

  if (nread < 0)
  {
    uv_link_close((uv_link_t *)observer, observer_connection_link_close_cb);
  }
}

void conn_init(uv_stream_t *handle)
{
  conn_t *conn = malloc(sizeof(conn_t));
  conn->proxy_handle = NULL;
  conn->handle = handle;

  CHECK(uv_link_source_init(&conn->source, (uv_stream_t *)conn->handle));
  conn->source.data = conn;
  CHECK(uv_link_init(&conn->http_link, &http_link_methods));
  CHECK(uv_link_observer_init(&conn->observer));
  http_link_init(&conn->http_link, &conn->http_link_context, server->config);
  CHECK(uv_link_chain((uv_link_t *)&conn->source, &conn->http_link));
  CHECK(uv_link_chain((uv_link_t *)&conn->http_link, (uv_link_t *)&conn->observer));

  conn->observer.observer_read_cb = observer_connection_read_cb;
  conn->observer.data = conn;
  CHECK(uv_link_read_start((uv_link_t *)&conn->observer));

  struct sockaddr_storage addr = {0};
  int alen = sizeof addr;
  uv_tcp_getpeername((uv_tcp_t *)conn->handle, (struct sockaddr *)&addr, &alen);
  if (addr.ss_family == AF_INET)
  {
    uv_ip4_name((const struct sockaddr_in *)&addr, conn->http_link_context.peer_ip, sizeof(conn->http_link_context.peer_ip));
  }
  else if (addr.ss_family == AF_INET6)
  {
    uv_ip6_name((const struct sockaddr_in6 *)&addr, conn->http_link_context.peer_ip, sizeof(conn->http_link_context.peer_ip));
  }
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

void proxy_close_cb(uv_handle_t *peer)
{
  free(peer);
}

void proxy_read_cb(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
{
  conn_t *conn = (conn_t *)handle->data;

  if (nread > 0)
  {
    uv_buf_t tmp_buf = uv_buf_init(buf->base, nread);
    int err = uv_link_write(&conn->http_link, &tmp_buf, 1, NULL, http_write_link_cb, buf->base);
    if (err)
    {
      log_error("error writing to client: %s", strerror(err));
    }
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
  if (nread <= 0)
  {
    free(buf->base);
  }
}

void proxy_connect_cb(uv_connect_t *req, int status)
{
  conn_t *conn = req->handle->data;
  free(req);

  if (status < 0)
  {
    uv_link_close((uv_link_t *)&conn->observer, observer_connection_link_close_cb);
    // TODO: write meaningful responses
    return;
  }

  uv_read_start((uv_stream_t *)conn->proxy_handle, alloc_cb, proxy_read_cb);
  http_request_t *request = &conn->http_link_context.request;
  write_buf((uv_stream_t *)conn->proxy_handle, request->raw, request->raw_len);
}

void proxy_http_request(char *ip, unsigned short port, conn_t *conn)
{
  struct sockaddr_in dest;
  uv_ip4_addr(ip, port, &dest);
  conn->proxy_handle = malloc(sizeof *conn->proxy_handle);
  memset(conn->proxy_handle, 0, sizeof *conn->proxy_handle);
  conn->proxy_handle->data = conn;

  uv_tcp_init(server->loop, conn->proxy_handle);
  uv_tcp_keepalive(conn->proxy_handle, 1, 60);

  uv_connect_t *connect_req = malloc(sizeof *connect_req);
  memset(connect_req, 0, sizeof *connect_req);
  uv_tcp_connect(connect_req, conn->proxy_handle,
                 (const struct sockaddr *)&dest, proxy_connect_cb);
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
