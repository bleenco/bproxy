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
  if (!f)
  {
    log_fatal("could not open config file: %s!", path);
    exit(1);
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *contents = malloc(fsize + 1);
  fread(contents, fsize, 1, f);
  fclose(f);
  contents[fsize] = '\0';

  return contents;
}

void parse_config(const char *json_string, config_t *config)
{
  const cJSON *port = NULL;
  const cJSON *secure_port = NULL;
  const cJSON *mime_type = NULL;
  const cJSON *mime_types = NULL;
  const cJSON *proxies = NULL;
  const cJSON *proxy = NULL;
  const cJSON *proxy_host = NULL;
  const cJSON *proxy_hosts = NULL;
  const cJSON *proxy_ip = NULL;
  const cJSON *proxy_port = NULL;
  const cJSON *log_file = NULL;
  const cJSON *certificate_path = NULL;
  const cJSON *key_path = NULL;

  memset(config, 0, sizeof *config);

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
  if (cJSON_IsNumber(port) && port->valueint)
  {
    config->port = port->valueint;
  }
  else
  {
    log_fatal("could not find listen port in configuration JSON!");
    cJSON_Delete(json);
    exit(1);
  }

  secure_port = cJSON_GetObjectItemCaseSensitive(json, "secure_port");
  if (cJSON_IsNumber(secure_port) && secure_port->valueint)
  {
    config->secure_port = secure_port->valueint;
  }
  else if(secure_port)
  {
    log_fatal("secure_port in wrong format in configuration JSON!");
    cJSON_Delete(json);
    exit(1);
  }

  certificate_path = cJSON_GetObjectItemCaseSensitive(json, "certificate_path");
  if (cJSON_IsString(certificate_path) && certificate_path->valuestring)
  {
    size_t len = strlen(certificate_path->valuestring);
    config->certificate_path = malloc(len +1);
    strcpy(config->certificate_path, certificate_path->valuestring);
    config->certificate_path[len] = '\0';
  }

  key_path = cJSON_GetObjectItemCaseSensitive(json, "key_path");
  if (cJSON_IsString(key_path) && key_path->valuestring)
  {
    size_t len = strlen(key_path->valuestring);
    config->key_path = malloc(len +1);
    strcpy(config->key_path, key_path->valuestring);
    config->key_path[len] = '\0';
  }

  log_file = cJSON_GetObjectItemCaseSensitive(json, "log_file");
  if (cJSON_IsString(log_file) && log_file->valuestring)
  {
    FILE *fp = fopen(log_file->valuestring, "w+");
    if (fp)
    {
      log_set_fp(fp);
    }
    else
    {
      log_error("cannot open file for writing: %s!", log_file->valuestring);
    }
  }

  config->num_gzip_mime_types = 0;
  mime_types = cJSON_GetObjectItemCaseSensitive(json, "gzip_mime_types");
  cJSON_ArrayForEach(mime_type, mime_types)
  {
    if (cJSON_IsString(mime_type) && mime_type->valuestring)
    {
      config->num_gzip_mime_types++;
      config->gzip_mime_types[config->num_gzip_mime_types - 1] = malloc(sizeof(char) * 100);
      memcpy(config->gzip_mime_types[config->num_gzip_mime_types - 1], mime_type->valuestring, strlen(mime_type->valuestring));
      config->gzip_mime_types[config->num_gzip_mime_types - 1][strlen(mime_type->valuestring)] = '\0';
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

  if(config->certificate_path && config->key_path && config->secure_port > 0)
  {
    config->ssl_enabled = true;
  }

  cJSON_Delete(json);
}
