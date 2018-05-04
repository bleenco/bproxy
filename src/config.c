/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/bproxy
 */
#include "config.h"
#include "log.h"

char *read_file(char *path)
{
  FILE *f = fopen(path, "rb");
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (!f)
  {
    log_error("could not open file %s for reading!", path);
    exit(1);
  }

  char *contents = malloc(fsize + 1);
  fread(contents, fsize, 1, f);
  fclose(f);
  contents[fsize] = '\0';

  return contents;
}

void parse_config(const char *json_string, config_t *config)
{
  const cJSON *port = NULL;
  const cJSON *mime_type = NULL;
  const cJSON *mime_types = NULL;
  const cJSON *proxies = NULL;
  const cJSON *proxy = NULL;
  const cJSON *proxy_host = NULL;
  const cJSON *proxy_hosts = NULL;
  const cJSON *proxy_ip = NULL;
  const cJSON *proxy_port = NULL;

  cJSON *json = cJSON_Parse(json_string);
  if (!json)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr)
    {
      log_fatal("could not parse configuration JSON, error before: %s\n", error_ptr);
    }

    cJSON_Delete(json);
    exit(1);
  }

  port = cJSON_GetObjectItemCaseSensitive(json, "port");
  if (cJSON_IsNumber(port) && port->valueint != NULL)
  {
    config->port = port->valueint;
  }
  else
  {
    log_fatal("could not find listen port in configuration JSON!");
    cJSON_Delete(json);
    exit(1);
  }

  config->num_gzip_mime_types = 0;
  mime_types = cJSON_GetObjectItemCaseSensitive(json, "mime_types");
  cJSON_ArrayForEach(mime_type, mime_types)
  {
    if (cJSON_IsString(mime_type) && mime_type->valuestring)
    {
      config->num_gzip_mime_types++;
      config->gzip_mime_types[config->num_gzip_mime_types - 1] = malloc(sizeof(char) * 100);
      memset(config->gzip_mime_types[config->num_gzip_mime_types - 1], 0, 100);
      memcpy(config->gzip_mime_types[config->num_gzip_mime_types - 1], mime_type->valuestring, strlen(mime_type->valuestring));
    }
  }

  config->num_proxies = 0;
  proxies = cJSON_GetObjectItemCaseSensitive(json, "proxies");
  cJSON_ArrayForEach(proxy, proxies)
  {
    config->num_proxies++;

    config->proxies[config->num_proxies - 1] = malloc(sizeof(proxy_config_t));
    proxy_hosts = cJSON_GetObjectItemCaseSensitive(proxy, "hosts");
    config->proxies[config->num_proxies - 1]->num_hosts = 0;
    cJSON_ArrayForEach(proxy_host, proxy_hosts)
    {
      if (cJSON_IsString(proxy_host) && proxy_host->valuestring)
      {
        config->proxies[config->num_proxies - 1]->num_hosts++;
        config->proxies[config->num_proxies - 1]->hosts[config->proxies[config->num_proxies - 1]->num_hosts - 1] = malloc(strlen(proxy_host->valuestring) + 1);
        memcpy(config->proxies[config->num_proxies - 1]->hosts[config->proxies[config->num_proxies - 1]->num_hosts - 1], proxy_host->valuestring, strlen(proxy_host->valuestring));
        config->proxies[config->num_proxies - 1]->hosts[config->proxies[config->num_proxies - 1]->num_hosts - 1][strlen(proxy_host->valuestring)] = '\0';
      }
    }

    proxy_ip = cJSON_GetObjectItemCaseSensitive(proxy, "ip");
    if (cJSON_IsString(proxy_ip) && proxy_ip->valuestring)
    {
      config->proxies[config->num_proxies - 1]->ip = malloc(20);
      memcpy(config->proxies[config->num_proxies - 1]->ip, proxy_ip->valuestring, strlen(proxy_ip->valuestring));
      config->proxies[config->num_proxies - 1]->ip[strlen(proxy_ip->valuestring)] = '\0';
    }

    proxy_port = cJSON_GetObjectItemCaseSensitive(proxy, "port");
    if (cJSON_IsNumber(proxy_port) && proxy_port->valueint)
    {
      config->proxies[config->num_proxies - 1]->port = proxy_port->valueint;
    }
  }

  cJSON_Delete(json);
}
