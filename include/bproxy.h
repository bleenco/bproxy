/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/vex
 */
#ifndef _BPROXY_BPROXY_H_
#define _BPROXY_BPROXY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include "http_link.h"
#include "config.h"
#include "version.h"

#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/evp.h"
#include "openssl/pem.h"
#include "openssl/x509.h"

#include "uv_ssl_t.h"

typedef struct
{
  uv_write_t req;
  uv_buf_t buf;
} write_req_t;

typedef struct server_t
{
  uv_loop_t *loop;
  uv_tcp_t tcp;
  uv_tcp_t secure_tcp;
  struct sockaddr_in address;
  config_t *config;
  int num_configs;
  char *config_file;
} server_t;

typedef struct conn_s
{
  uv_stream_t *handle;
  bool handle_flushed;
  uv_tcp_t *proxy_handle;
  http_link_context_t http_link_context;
  QUEUE raw_requests;

  uv_link_source_t source;
  uv_link_t http_link;
  uv_link_observer_t observer;

  SSL *ssl;
  uv_ssl_t *ssl_link;
} conn_t;

server_t *server;
static SSL_CTX *default_ctx;

static void conn_init(uv_stream_t *handle);
static void conn_close(conn_t *conn);

static void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void write_buf(uv_stream_t *handle, char *data, int len);

void proxy_close_cb(uv_handle_t *peer);
void proxy_read_cb(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf);
void proxy_connect_cb(uv_connect_t *req, int status);
void proxy_http_request(char *ip, unsigned short port, conn_t *conn);

static void write_cb(uv_write_t *req, int status);
void link_close_cb(uv_link_t *source);
static void shutdown_cb(uv_shutdown_t *req, int status);
static void connection_cb(uv_stream_t *s, int status);

EVP_PKEY *generatePrivateKey();
X509 *generateCertificate(EVP_PKEY *pkey);

static proxy_config_t *find_proxy_config(const char *hostname);
static int server_init();
static int server_listen(unsigned short port, uv_tcp_t *tcp);
void parse_args(int argc, char **argv);
void usage();

#endif // _BPROXY_BPROXY_H_
