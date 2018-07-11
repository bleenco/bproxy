/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/bproxy
 */
#ifndef _BPROXY_CONFIG_H_
#define _BPROXY_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cJSON.h"
#include "log.h"

#include "openssl/ssl.h"
#include "openssl/err.h"
#include "uv_ssl_t.h"

#define CONFIG_MAX_HOSTS 10
#define CONFIG_MAX_GZIP_MIME_TYPES 20
#define CONFIG_MAX_PROXIES 100

typedef struct proxy_config_t
{
  char *hosts[CONFIG_MAX_HOSTS];
  char *ip;
  unsigned short port;
  int num_hosts;
  SSL_CTX *ssl_context;
  bool ssl_passtrough;
} proxy_config_t;

typedef struct config_t
{
  unsigned short port;
  unsigned short secure_port;
  char *gzip_mime_types[CONFIG_MAX_GZIP_MIME_TYPES];
  int num_gzip_mime_types;
  proxy_config_t *proxies[CONFIG_MAX_PROXIES];
  int num_proxies;
} config_t;

char *read_file(char *path);
void parse_config(const char *json_string, config_t *config);

#endif // _BPROXY_CONFIG_H_
