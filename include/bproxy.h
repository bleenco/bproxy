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

#include "uv.h"
#include "http_parser.h"
#include "http.h"
#include "config.h"
#include "version.h"
#include "gzip.h"

typedef struct
{
  uv_write_t req;
  uv_buf_t buf;
} write_req_t;

typedef struct server_t
{
  uv_loop_t *loop;
  uv_tcp_t tcp;
  struct sockaddr_in address;
  config_t *config;
  int num_configs;
  char *config_file;
} server_t;

typedef struct proxy_t
{
  int fd;
  conn_t *conn;
  uv_stream_t *handle;
  uv_tcp_t tcp;
  uv_connect_t connect_req;
  uv_shutdown_t shutdown_req;
  uv_write_t write_req;
  gzip_state_t *gzip_state;
  http_response_t response;
} proxy_t;

typedef struct proxy_ip_port
{
  char *ip;
  unsigned short port;
} proxy_ip_port;

server_t *server;

static void conn_init(uv_stream_t *handle);
static void conn_free(conn_t* conn);

static void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void write_buf(uv_stream_t *handle, char *data, int len);

void proxy_close_cb(uv_handle_t *peer);
void proxy_read_cb(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf);
void proxy_connect_cb(uv_connect_t *req, int status);
void proxy_http_request(char *ip, unsigned short port, conn_t *conn);

static void write_cb(uv_write_t *req, int status);
//static void close_cb(uv_handle_t *peer);
void link_close_cb(uv_link_t* source);
static void shutdown_cb(uv_shutdown_t *req, int status);
//static void read_cb(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf);
static void connection_cb(uv_stream_t *s, int status);

static proxy_ip_port find_proxy_config(char *hostname);
static int server_init();
void parse_args(int argc, char **argv);
void usage();

uv_link_methods_t middle_methods;


#endif // _BPROXY_BPROXY_H_
