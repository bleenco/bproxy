/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/bproxy
 */
#ifndef _BPROXY_HTTP_H_
#define _BPROXY_HTTP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "uv.h"
#include "http_parser.h"
#include "version.h"

#define MAX_HEADERS 20
#define MAX_ELEMENT_SIZE 500

typedef bool boolean;

enum header_element
{
  NONE = 0,
  FIELD,
  VALUE
};

typedef struct http_request_t
{
  char *raw;
  enum http_method method;
  char *host;
  char *url;
  char *body;
  char *hostname;
  uint8_t http_major;
  uint8_t http_minor;
  uint8_t keepalive;
  uint8_t upgrade;
  enum header_element last_header_element;
  int num_headers;
  char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE];
  boolean enable_compression;
} http_request_t;

typedef struct http_response_t
{
  http_parser parser;
  char *raw_body;

  int expected_data_len;
  int processed_data_len;

  size_t body_size;

  enum header_element last_header_element;
  int num_headers;
  char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE];
  char status_line[256];
  boolean enable_compression;
} http_response_t;

typedef struct conn_t
{
  int fd;
  http_parser *parser;
  http_request_t *request;
  uv_stream_t *handle;
  boolean ws_handshake_sent;
  uv_stream_t *proxy_handle;
} conn_t;

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
void http_404_response(char *resp);

// clang-format off
static http_parser_settings parser_settings =
{
  .on_message_begin = message_begin_cb,
  .on_header_field = headers_field_cb,
  .on_header_value = headers_value_cb,
  .on_url = url_cb,
  .on_headers_complete = headers_complete_cb,
  .on_message_complete = message_complete_cb };

static http_parser_settings resp_parser_settings =
{
  .on_message_begin = response_message_begin_cb,
  .on_header_field = response_headers_field_cb,
  .on_header_value = response_headers_value_cb,
  .on_headers_complete = response_headers_complete_cb,
  .on_body = body_cb };
// clang-format on

#endif // _BPROXY_HTTP_H_
