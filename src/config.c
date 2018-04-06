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

static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0)
  {
    return 0;
  }
  return -1;
}

void parse_config(const char *json_string, config_t *config)
{
  jsmn_parser p;
  jsmntok_t t[1024];

  jsmn_init(&p);
  int r = jsmn_parse(&p, json_string, strlen(json_string), t, sizeof(t) / sizeof(t[0]));
  if (r < 0)
  {
    log_error("failed to parse config JSON: %d", r);
    exit(1);
  }

  // assume top-level element is an object
  if (r < 1 || t[0].type != JSMN_OBJECT)
  {
    log_error("wrong JSON configuration!");
    exit(1);
  }

  for (int i = 1; i < r; i++)
  {
    if (jsoneq(json_string, &t[i], "port") == 0)
    {
      char *port = malloc(7);
      unsigned short portint;
      sprintf(port, "%.*s", t[i + 1].end - t[i + 1].start, json_string + t[i + 1].start);
      portint = atoi(port);
      config->port = portint;
      free(port);
      i++;
    }
    else if (jsoneq(json_string, &t[i], "gzip_mime_types") == 0)
    {
      if (t[i + 1].type != JSMN_ARRAY)
      {
        log_error("wrong configuration, gzip_mime_types should be an array!");
        exit(1);
      }

      config->num_gzip_mime_types = 0;
      for (int j = 0; j < t[i + 1].size; j++)
      {
        jsmntok_t *mime_type = &t[i + j + 2];
        char *mime = malloc(50);
        sprintf(mime, "%.*s", mime_type->end - mime_type->start, json_string + mime_type->start);
        config->gzip_mime_types[config->num_gzip_mime_types] = malloc(sizeof(char) * 50);
        memcpy(config->gzip_mime_types[config->num_gzip_mime_types], mime, strlen(mime));
        config->num_gzip_mime_types++;
      }
    }
    else if (jsoneq(json_string, &t[i], "proxies") == 0)
    {
      if (t[i + 1].type != JSMN_ARRAY)
      {
        log_error("wrong configuration, proxies should be an array!");
        exit(1);
      }

      jsmntok_t *proxy = &t[i + 1];
      jsmntok_t token[300];
      jsmn_parser pp;
      char *proxy_str = malloc(1024 * 100);
      sprintf(proxy_str, "%.*s\n", proxy->end - proxy->start, json_string + proxy->start);

      jsmn_init(&pp);
      int rr = jsmn_parse(&pp, proxy_str, strlen(proxy_str), token, sizeof(token) / sizeof(token[0]));
      if (rr < 0)
      {
        log_error("failed to parse JSON (proxies): %d", rr);
        exit(1);
      }

      config->num_proxies = 0;
      for (int x = 1, it = 0; x < rr; x++)
      {
        if (jsoneq(proxy_str, &token[x], "hosts") == 0)
        {
          config->num_proxies++;
          config->proxies[it] = malloc(sizeof(proxy_config_t));

          config->proxies[it]->num_hosts = token[x + 1].size;
          for (int y = 0; y < token[x + 1].size; y++)
          {
            jsmntok_t *h = &token[x + y + 2];
            char *host = malloc(50);
            sprintf(host, "%.*s", h->end - h->start, proxy_str + h->start);
            config->proxies[it]->hosts[y] = malloc(strlen(host) + 1);
            memcpy(config->proxies[it]->hosts[y], host, strlen(host));
            config->proxies[it]->hosts[y][strlen(host)] = '\0';
            free(host);
          }

          x += token[x + 1].size + 1;
        }

        if (jsoneq(proxy_str, &token[x], "ip") == 0)
        {
          char *ip = malloc(20);
          sprintf(ip, "%.*s", token[x + 1].end - token[x + 1].start, proxy_str + token[x + 1].start);
          config->proxies[it]->ip = malloc(20);
          memcpy(config->proxies[it]->ip, ip, strlen(ip));
          config->proxies[it]->ip[strlen(ip)] = '\0';
          free(ip);
          x++;
        }

        if (jsoneq(proxy_str, &token[x], "port") == 0)
        {
          char *port = malloc(7);
          unsigned short portint;
          sprintf(port, "%.*s", token[x + 1].end - token[x + 1].start, proxy_str + token[x + 1].start);
          portint = atoi(port);
          config->proxies[it]->port = portint;
          free(port);
          x++;

          it++;
        }
      }

      free(proxy_str);
      return;
    }
  }
}
