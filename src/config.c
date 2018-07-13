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
  const cJSON *ssl_passthrough = NULL;
  const cJSON *force_ssl = NULL;

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
  else if (secure_port)
  {
    log_fatal("secure_port in wrong format in configuration JSON!");
    cJSON_Delete(json);
    exit(1);
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
    proxy_config_t* proxy_config = config->proxies[config->num_proxies - 1];
    memset(proxy_config, 0, sizeof(proxy_config_t));

    proxy_hosts = cJSON_GetObjectItemCaseSensitive(proxy, "hosts");
    proxy_config->num_hosts = 0;
    cJSON_ArrayForEach(proxy_host, proxy_hosts)
    {
      if (cJSON_IsString(proxy_host) && proxy_host->valuestring)
      {
        proxy_config->num_hosts++;
        proxy_config->hosts[proxy_config->num_hosts - 1] = malloc(strlen(proxy_host->valuestring) + 1);
        memcpy(proxy_config->hosts[proxy_config->num_hosts - 1], proxy_host->valuestring, strlen(proxy_host->valuestring));
        proxy_config->hosts[proxy_config->num_hosts - 1][strlen(proxy_host->valuestring)] = '\0';
      }
    }

    proxy_ip = cJSON_GetObjectItemCaseSensitive(proxy, "ip");
    if (cJSON_IsString(proxy_ip) && proxy_ip->valuestring)
    {
      proxy_config->ip = malloc(20);
      memcpy(proxy_config->ip, proxy_ip->valuestring, strlen(proxy_ip->valuestring));
      proxy_config->ip[strlen(proxy_ip->valuestring)] = '\0';
    }

    proxy_port = cJSON_GetObjectItemCaseSensitive(proxy, "port");
    if (cJSON_IsNumber(proxy_port) && proxy_port->valueint)
    {
      proxy_config->port = proxy_port->valueint;
    }

    bool ssl_enabled = config->secure_port > 0;

    certificate_path = cJSON_GetObjectItemCaseSensitive(proxy, "certificate_path");
    if (!cJSON_IsString(certificate_path) || !certificate_path->valuestring)
    {
      ssl_enabled = false;
    }

    key_path = cJSON_GetObjectItemCaseSensitive(proxy, "key_path");
    if (!cJSON_IsString(key_path) || !key_path->valuestring)
    {
      ssl_enabled = false;
    }

    if (ssl_enabled)
    {
      proxy_config->ssl_context = SSL_CTX_new(SSLv23_method());
      if (!SSL_CTX_use_certificate_chain_file(proxy_config->ssl_context, certificate_path->valuestring))
      {
        int err = ERR_get_error();
        log_error("Could not load certificate file: %s; reason: %s", certificate_path->valuestring, ERR_error_string(err, NULL));
        ssl_enabled = false;
      }
      if (ssl_enabled && !SSL_CTX_use_PrivateKey_file(proxy_config->ssl_context, key_path->valuestring, SSL_FILETYPE_PEM))
      {
        int err = ERR_get_error();
        log_error("Could not load key file: %s or key doesn't match certificate: %s; reason: %s", key_path->valuestring, certificate_path->valuestring, ERR_error_string(err, NULL));
        ssl_enabled = false;
      }
    }
    if (ssl_enabled)
    {
      if (uv_ssl_setup_recommended_secure_context(proxy_config->ssl_context))
      {
        log_error("configuring recommended secure context");
        ssl_enabled = false;
      }
    }

    force_ssl = cJSON_GetObjectItemCaseSensitive(proxy, "force_ssl");
    if (cJSON_IsBool(force_ssl))
    {
      proxy_config->force_ssl = force_ssl->type == cJSON_True;
    }

    ssl_passthrough = cJSON_GetObjectItemCaseSensitive(proxy, "ssl_passthrough");
    if (cJSON_IsBool(ssl_passthrough))
    {
      if (ssl_enabled)
      {
        log_warn("ssl_passthrough enabled, certificate and key file will be ignored!");
        ssl_enabled = false;
      }
      proxy_config->ssl_passthrough = ssl_passthrough->type == cJSON_True;
      proxy_config->force_ssl = proxy_config->ssl_passthrough ? true : proxy_config->force_ssl;
    }

    if (!ssl_enabled)
    {
      SSL_CTX_free(proxy_config->ssl_context);
      proxy_config->ssl_context = NULL;
      force_ssl = false;
    }

  }

  cJSON_Delete(json);
}
