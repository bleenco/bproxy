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

#include "jsmn.h"

#define CONFIG_MAX_HOSTS 10

typedef struct proxy_config_t
{
  char *hosts[CONFIG_MAX_HOSTS];
  char *ip;
  unsigned short port;
  int num_hosts;
} proxy_config_t;

typedef struct config_t
{
  unsigned short port;
  proxy_config_t *proxies[100];
  int num_proxies;
} config_t;

char *read_file(char *path);
void parse_config(const char *json_string, config_t *config);

#endif // _BPROXY_CONFIG_H_
