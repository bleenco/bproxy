/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/bproxy
 */
#include "bproxy.h"
#include "log.h"
#include <arpa/inet.h>

#include <assert.h>

#define CHECK(V) \
  if ((V) != 0)  \
  abort()

#define CHECK_ALLOC(V) \
  if ((V) == NULL)     \
  abort()

static void write_link_cb(uv_link_t *source, int status, void *arg)
{
  free(arg);
}

static void observer_connection_link_shutdown_cb(uv_link_t *source, int status, void *arg)
{
  conn_t *conn;
  conn = source->data;
  conn->handle_flushed = true;
  conn_close(conn);
}

static void client_connection_link_close_cb(uv_link_t *link)
{
  conn_t *conn;
  conn = link->data;
  SSL_free(conn->ssl);
  conn->ssl = NULL;
  free(conn->handle);
  conn->handle = NULL;
  conn_close(conn);
}

void free_raw_requests_queue(conn_t *conn)
{
  while (!QUEUE_EMPTY(&conn->raw_requests))
  {
    buf_queue_t *bq = QUEUE_DATA(QUEUE_NEXT(&conn->raw_requests), buf_queue_t, member);
    QUEUE_REMOVE(&bq->member);
    free(bq);
  }
}

static void client_connection_read_cb(uv_link_t *observer, ssize_t nread,
                                      const uv_buf_t *buf)
{
  conn_t *conn = (conn_t *)observer->data;
  if (nread > 0)
  {
    buf_queue_t *buf_queue_body_node = malloc(sizeof *buf_queue_body_node);
    buf_queue_body_node->buf.base = buf->base;
    buf_queue_body_node->buf.len = nread;
    QUEUE_INIT(&buf_queue_body_node->member);
    QUEUE_INSERT_TAIL(&conn->raw_requests, &buf_queue_body_node->member);

    if (conn->proxy_handle)
    {
      QUEUE *q;
      QUEUE_FOREACH(q, &conn->raw_requests)
      {
        buf_queue_t *bq = QUEUE_DATA(q, buf_queue_t, member);
        write_buf((uv_stream_t *)conn->proxy_handle, bq->buf.base, bq->buf.len);
        free(bq->buf.base);
      }
      free_raw_requests_queue(conn);
    }
    else
    {
      proxy_config_t *proxy_config = find_proxy_config(conn->http_link_context.request.hostname);
      if (!proxy_config)
      {
        char *resp = malloc(strlen(server->config->templates->status_404_template) * sizeof(char));
        strncpy(resp, server->config->templates->status_404_template, strlen(server->config->templates->status_404_template));
        uv_buf_t tmp_buf = uv_buf_init(resp, strlen(resp));
        uv_link_write((uv_link_t *)observer, &tmp_buf, 1, NULL, write_link_cb, resp);
        return;
      }
      else if (proxy_config->force_ssl && !conn->http_link_context.https)
      {
        char *resp = malloc(4096 * sizeof(char));
        http_301_response(resp, &conn->http_link_context.request, server->config->secure_port);
        uv_buf_t tmp_buf = uv_buf_init(resp, strlen(resp));
        uv_link_write((uv_link_t *)observer, &tmp_buf, 1, NULL, write_link_cb, resp);
        return;
      }
      conn->config = proxy_config;
      proxy_http_request(proxy_config->ip, proxy_config->port, conn);
    }
  }

  if (nread < 0)
  {
    if (nread == -400)
    {
      char *resp = malloc(strlen(server->config->templates->status_400_template) * sizeof(char));
      strncpy(resp, server->config->templates->status_400_template, strlen(server->config->templates->status_400_template));
      uv_buf_t tmp_buf = uv_buf_init(resp, strlen(resp));
      uv_link_write((uv_link_t *)observer, &tmp_buf, 1, NULL, write_link_cb, resp);
    }
    else
    {
      conn_close(conn);
    }
  }
  if (nread <= 0)
  {
    if (buf)
    {
      free(buf->base);
    }
  }
}

static int ssl_servername_cb(SSL *s, int *ad, void *arg)
{
  const char *hostname = SSL_get_servername(s, TLSEXT_NAMETYPE_host_name);
  if (!hostname || hostname[0] == '\0')
  {
    return SSL_TLSEXT_ERR_NOACK;
  }
  proxy_config_t *proxy_config = find_proxy_config(hostname);
  if (!proxy_config)
  {
    SSL_set_SSL_CTX(s, default_ctx);
    return SSL_TLSEXT_ERR_NOACK;
  }
  if (proxy_config && proxy_config->ssl_context)
  {
    SSL_set_SSL_CTX(s, proxy_config->ssl_context);
  }
  else if (proxy_config && proxy_config->ssl_passthrough)
  {
    conn_t *conn = arg;
    strcpy(conn->http_link_context.request.hostname, hostname);

    uv_link_unchain((uv_link_t *)conn->ssl_link, &conn->http_link);
    uv_link_unchain(&conn->http_link, (uv_link_t *)&conn->observer);
    uv_link_chain((uv_link_t *)conn->ssl_link, (uv_link_t *)&conn->observer);

    uv_ssl_cancel(conn->ssl_link);
  }
  else
  {
    log_error("SSL/TLS not configured properly for: %s", hostname);
    SSL_set_SSL_CTX(s, default_ctx);
    return SSL_TLSEXT_ERR_OK;
  }
  return SSL_TLSEXT_ERR_OK;
}

void conn_init(uv_stream_t *handle)
{
  int err = 0;
  bool ssl_conn = false;

  conn_t *conn = malloc(sizeof(conn_t));
  memset(conn, 0, sizeof *conn);
  conn->handle = handle;

  QUEUE_INIT(&conn->raw_requests);

  CHECK(uv_link_source_init(&conn->source, (uv_stream_t *)conn->handle));
  conn->source.data = conn;

  CHECK(uv_link_init(&conn->observer, &proxy_methods));
  CHECK(uv_link_init(&conn->http_link, &http_link_methods));
  http_link_init(&conn->http_link, &conn->http_link_context, server->config);

  // Get remote address
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
  // Get local port
  alen = sizeof addr;
  uv_tcp_getsockname((uv_tcp_t *)conn->handle, (struct sockaddr *)&addr, &alen);
  if (addr.ss_family == AF_INET)
  {
    ssl_conn = ntohs(((const struct sockaddr_in *)&addr)->sin_port) == server->config->secure_port;
  }
  else if (addr.ss_family == AF_INET6)
  {
    ssl_conn = ntohs(((const struct sockaddr_in6 *)&addr)->sin6_port) == server->config->secure_port;
  }

  conn->http_link_context.https = ssl_conn;

  if (ssl_conn)
  {
    CHECK_ALLOC(conn->ssl = SSL_new(default_ctx));
    SSL_CTX_set_tlsext_servername_callback(default_ctx, ssl_servername_cb);
    SSL_set_accept_state(conn->ssl);
    CHECK_ALLOC(conn->ssl_link = uv_ssl_create(uv_default_loop(), conn->ssl, &err));
    CHECK(err);
    ((uv_link_t *)conn->ssl_link)->data = conn;
    CHECK(uv_link_chain((uv_link_t *)&conn->source, (uv_link_t *)conn->ssl_link));
    CHECK(uv_link_chain((uv_link_t *)conn->ssl_link, &conn->http_link));
  }
  else
  {
    CHECK(uv_link_chain((uv_link_t *)&conn->source, &conn->http_link));
  }
  CHECK(uv_link_chain((uv_link_t *)&conn->http_link, (uv_link_t *)&conn->observer));

  conn->observer.data = conn;
  CHECK(uv_link_read_start((uv_link_t *)&conn->observer));
}

void conn_close(conn_t *conn)
{
  if (conn->proxy_handle)
  {
    if (!uv_is_closing((uv_handle_t *)conn->proxy_handle))
    {
      uv_close((uv_handle_t *)conn->proxy_handle, proxy_close_cb);
    }
  }
  if (conn->handle)
  {
    if (!uv_is_closing((uv_handle_t *)conn->handle))
    {
      if (!conn->handle_flushed)
      {
        uv_link_shutdown((uv_link_t *)&conn->observer, observer_connection_link_shutdown_cb, NULL);
      }
      else
      {
        uv_link_close((uv_link_t *)&conn->observer, client_connection_link_close_cb);
      }
    }
  }
  if (!conn->handle && !conn->proxy_handle)
  {
    QUEUE *q;
    QUEUE_FOREACH(q, &conn->raw_requests)
    {
      buf_queue_t *bq = QUEUE_DATA(q, buf_queue_t, member);
      free(bq->buf.base);
    }
    free_raw_requests_queue(conn);
    free(conn);
  }
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
    log_error("error writing to destination!");
    return;
  }
  write_req_t *wr = (write_req_t *)req->data;
  free(wr->buf.base);
  free(wr);
}

void proxy_close_cb(uv_handle_t *peer)
{
  conn_t *conn = peer->data;
  conn->proxy_handle = NULL;
  free(peer);

  free_raw_requests_queue(conn);
  conn_close(conn);
}

void proxy_read_cb(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
{
  conn_t *conn = (conn_t *)handle->data;

  if (nread > 0)
  {
    // Set keep alive for websockets
    if (conn->http_link_context.type == TYPE_WEBSOCKET && conn->http_link_context.initial_reply)
    {
      uv_tcp_keepalive((uv_tcp_t *)conn->handle, true, 0);
      uv_tcp_keepalive((uv_tcp_t *)conn->proxy_handle, true, 0);
    }

    uv_buf_t tmp_buf = uv_buf_init(buf->base, nread);
    int err = uv_link_write((uv_link_t *)&conn->observer, &tmp_buf, 1, NULL, write_link_cb, buf->base);
    if (err)
    {
      log_error("error writing to client: %s", uv_err_name(err));
      conn_close(conn);
    }
  }
  else if (nread < 0)
  {
    if (nread != UV_EOF)
    {
      log_error("could not read from socket! (%s)", uv_strerror(nread));
    }
    conn_close(conn);
  }
  if (nread <= 0)
  {
    free(buf->base);
  }
}

void proxy_connect_cb(uv_connect_t *req, int status)
{
  conn_t *conn = req->handle->data;
  QUEUE *q;
  free(req);

  if (status < 0)
  {
    QUEUE_FOREACH(q, &conn->raw_requests)
    {
      buf_queue_t *bq = QUEUE_DATA(q, buf_queue_t, member);
      free(bq->buf.base);
    }
    free_raw_requests_queue(conn);
    if (conn->config->ssl_passthrough)
    {
      conn_close(conn);
    }
    else
    {
      char *resp = malloc(strlen(server->config->templates->status_502_template) * sizeof(char));
      strncpy(resp, server->config->templates->status_502_template, strlen(server->config->templates->status_502_template));
      uv_buf_t tmp_buf = uv_buf_init(resp, strlen(resp));
      uv_link_write((uv_link_t *)&conn->observer, &tmp_buf, 1, NULL, write_link_cb, resp);
    }
    return;
  }

  uv_read_start((uv_stream_t *)conn->proxy_handle, alloc_cb, proxy_read_cb);
  QUEUE_FOREACH(q, &conn->raw_requests)
  {
    buf_queue_t *bq = QUEUE_DATA(q, buf_queue_t, member);
    write_buf((uv_stream_t *)conn->proxy_handle, bq->buf.base, bq->buf.len);
    free(bq->buf.base);
  }
  free_raw_requests_queue(conn);
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

EVP_PKEY *generatePrivateKey()
{
  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
  EVP_PKEY_keygen_init(pctx);
  EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 2048);
  EVP_PKEY_keygen(pctx, &pkey);
  return pkey;
}

X509 *generateCertificate(EVP_PKEY *pkey)
{
  X509 *x509 = X509_new();
  X509_set_version(x509, 2);
  ASN1_INTEGER_set(X509_get_serialNumber(x509), 0);
  X509_gmtime_adj(X509_get_notBefore(x509), 0);
  X509_gmtime_adj(X509_get_notAfter(x509), (long)60 * 60 * 24 * 365);
  X509_set_pubkey(x509, pkey);

  X509_NAME *name = X509_get_subject_name(x509);
  X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char *)"SI", -1, -1, 0);
  X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char *)"localhost", -1, -1, 0);
  X509_NAME_add_entry_by_txt(name, "distinguished_name", MBSTRING_ASC, (const unsigned char *)"dn", -1, -1, 0);
  X509_NAME_add_entry_by_txt(name, "keyUsage", MBSTRING_ASC, (const unsigned char *)"digitalSignature", -1, -1, 0);
  X509_NAME_add_entry_by_txt(name, "extendedKeyUsage", MBSTRING_ASC, (const unsigned char *)"serverAuth", -1, -1, 0);
  X509_set_issuer_name(x509, name);
  X509_sign(x509, pkey, EVP_sha256());
  return x509;
}

proxy_config_t *find_proxy_config(const char *hostname)
{
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
          return pconf;
        }
      }
    }
  }

  return NULL;
}

int server_listen(unsigned short port, uv_tcp_t *tcp)
{
  if (uv_tcp_init(server->loop, tcp))
  {
    log_error("cannot init tcp connection!");
    return 1;
  }
  uv_ip4_addr("0.0.0.0", port, &server->address);
  if (uv_tcp_bind(tcp, (const struct sockaddr *)&server->address, 0))
  {
    log_error("cannot bind server! check your permissions and another service running on same port.");
    return 1;
  }
  if (uv_listen((uv_stream_t *)tcp, 4096, connection_cb))
  {
    log_error("server listen error!");
    return 1;
  }
  log_info("listening on 0.0.0.0:%d", port);
  return 0;
}

int server_init()
{
  server->loop = uv_default_loop();

  server_listen(server->config->port, &server->tcp);

  if (server->config->secure_port > 0)
  {
    // Initialize SSL_CTX
    CHECK_ALLOC(default_ctx = SSL_CTX_new(SSLv23_method()));
    CHECK(uv_ssl_setup_recommended_secure_context(default_ctx));

    EVP_PKEY *pkey = generatePrivateKey();
    X509 *x509 = generateCertificate(pkey);

    SSL_CTX_use_certificate(default_ctx, x509);
    SSL_CTX_use_PrivateKey(default_ctx, pkey);

    RSA *rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);
    SSL_CTX_set_tmp_rsa(default_ctx, rsa);
    RSA_free(rsa);

    SSL_CTX_set_verify(default_ctx, SSL_VERIFY_NONE, 0);

    server_listen(server->config->secure_port, &server->secure_tcp);
  }

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
    free(json);
  }
}

static void ignore_sigpipe(void)
{
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  act.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &act, NULL);
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
  ignore_sigpipe();
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  OpenSSL_add_all_digests();
  SSL_load_error_strings();
  ERR_load_crypto_strings();

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

// clang-format off
uv_link_methods_t proxy_methods =
{
  .read_start = uv_link_default_read_start,
  .read_stop = uv_link_default_read_stop,
  .write = uv_link_default_write,
  .try_write = uv_link_default_try_write,
  .shutdown = uv_link_default_shutdown,
  .close = uv_link_default_close,

  .alloc_cb_override = uv_link_default_alloc_cb_override,
  .read_cb_override = client_connection_read_cb
};
// clang-format on
