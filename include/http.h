/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/bproxy
 */
#ifndef _BPROXY_HTTP_H_
#define _BPROXY_HTTP_H_

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "http_parser.h"
#include "uv.h"
#include "version.h"

#include "gzip.h"
#include "queue.h"

#define MAX_HEADERS 20
#define MAX_ELEMENT_SIZE 500

typedef bool boolean;

enum header_element { NONE = 0, FIELD, VALUE };

typedef struct buf_queue_s {
  uv_buf_t buf;
  QUEUE member;
} buf_queue_t;

typedef struct http_request_s {
  ssize_t raw_len;
  enum http_method method;
  char host[256];
  char *url;
  char *body;
  char hostname[256];
  uint8_t http_major;
  uint8_t http_minor;
  uint8_t keepalive;
  uint8_t upgrade;
  enum header_element last_header_element;
  int num_headers;
  char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE];
  char http_header[MAX_HEADERS * 2 * (MAX_ELEMENT_SIZE + 2) + 256];
  int http_header_len;
  boolean enable_compression;
  http_parser parser;
  int content_length;
  bool complete;
  char *status_line;
} http_request_t;

typedef struct http_response_s {
  http_parser parser;
  char *raw_body;
  size_t body_size;

  int expected_data_len;
  int processed_data_len;

  enum header_element last_header_element;
  int num_headers;
  char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE];
  char http_header[MAX_HEADERS * 2 * (MAX_ELEMENT_SIZE + 2) + 256];
  int http_header_len;
  char status_line[256];
  boolean enable_compression;
  boolean headers_received;
  boolean headers_send;
  gzip_state_t *gzip_state;
} http_response_t;

typedef struct http_link_context_s {
  http_request_t request;
  http_response_t response;
  config_t *server_config;  // TODO: Move this out, and use only part of
                            // configuration needed
  bool https;
  enum { TYPE_REQUEST, TYPE_WEBSOCKET } type;
  bool initial_reply;
  char peer_ip[45];

  // Data for logging
  uint64_t request_time;
  time_t request_timestamp;
} http_link_context_t;

int message_begin_cb(http_parser *p);
int headers_complete_cb(http_parser *p);
int url_cb(http_parser *p, const char *buf, size_t length);
int headers_field_cb(http_parser *p, const char *buf, size_t length);
int headers_value_cb(http_parser *p, const char *buf, size_t length);
int body_cb(http_parser *p, const char *buf, size_t length);
int message_complete_cb(http_parser *p);

int response_message_begin_cb(http_parser *p);
int response_headers_complete_cb(http_parser *p);
int response_headers_field_cb(http_parser *p, const char *buf, size_t length);
int response_headers_value_cb(http_parser *p, const char *buf, size_t length);

void parse_requested_host(http_request_t *request);
int insert_header(char *src, char *resp);
void insert_substring(char *a, char *b, int position);
char *substring(char *string, int position, int length);
void http_301_response(char *resp, const http_request_t *request,
                       unsigned short port);

void http_init_response_headers(http_response_t *response, bool compressed);
void http_init_request_headers(http_link_context_t *context);

// clang-format off
static http_parser_settings parser_settings =
{
  .on_message_begin = message_begin_cb,
  .on_header_field = headers_field_cb,
  .on_header_value = headers_value_cb,
  .on_url = url_cb,
  .on_headers_complete = headers_complete_cb,
  .on_message_complete = message_complete_cb
};

static http_parser_settings resp_parser_settings =
{
  .on_message_begin = response_message_begin_cb,
  .on_header_field = response_headers_field_cb,
  .on_header_value = response_headers_value_cb,
  .on_headers_complete = response_headers_complete_cb,
};
// clang-format on

#endif  // _BPROXY_HTTP_H_
