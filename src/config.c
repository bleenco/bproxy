/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/bproxy
 */
#include "config.h"
#include "log.h"

char *read_file(char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    log_fatal("could not open file to read: %s!", path);
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

void parse_config(const char *json_string, config_t *config) {
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
  const cJSON *templates = NULL;
  const cJSON *status_400_template = NULL;
  const cJSON *status_404_template = NULL;
  const cJSON *status_502_template = NULL;

  memset(config, 0, sizeof *config);

  cJSON *json = cJSON_Parse(json_string);
  if (!json) {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr) {
      log_fatal("could not parse configuration JSON, error before: %s\n",
                error_ptr);
    }

    cJSON_Delete(json);
    exit(1);
  }

  port = cJSON_GetObjectItemCaseSensitive(json, "port");
  if (cJSON_IsNumber(port) && port->valueint) {
    config->port = port->valueint;
  } else {
    log_fatal("could not find listen port in configuration JSON!");
    cJSON_Delete(json);
    exit(1);
  }

  secure_port = cJSON_GetObjectItemCaseSensitive(json, "secure_port");
  if (cJSON_IsNumber(secure_port) && secure_port->valueint) {
    config->secure_port = secure_port->valueint;
  } else if (secure_port) {
    log_fatal("secure_port in wrong format in configuration JSON!");
    cJSON_Delete(json);
    exit(1);
  }

  log_file = cJSON_GetObjectItemCaseSensitive(json, "log_file");
  if (cJSON_IsString(log_file) && log_file->valuestring) {
    FILE *fp = fopen(log_file->valuestring, "w+");
    if (fp) {
      log_set_fp(fp);
    } else {
      log_error("cannot open file for writing: %s!", log_file->valuestring);
    }
  }

  config->num_gzip_mime_types = 0;
  mime_types = cJSON_GetObjectItemCaseSensitive(json, "gzip_mime_types");
  cJSON_ArrayForEach(mime_type, mime_types) {
    if (cJSON_IsString(mime_type) && mime_type->valuestring) {
      config->num_gzip_mime_types++;
      config->gzip_mime_types[config->num_gzip_mime_types - 1] =
          malloc(sizeof(char) * 100);
      memcpy(config->gzip_mime_types[config->num_gzip_mime_types - 1],
             mime_type->valuestring, strlen(mime_type->valuestring));
      config->gzip_mime_types[config->num_gzip_mime_types - 1]
                             [strlen(mime_type->valuestring)] = '\0';
    }
  }

  config->templates = malloc(sizeof(templates_t));
  templates = cJSON_GetObjectItemCaseSensitive(json, "templates");

  status_400_template =
      cJSON_GetObjectItemCaseSensitive(templates, "status_400_template");
  if (!cJSON_IsString(status_400_template) ||
      !status_400_template->valuestring ||
      strcmp(status_400_template->valuestring, "") == 0) {
    char *contents = malloc(4096 * sizeof(char));
    http_400_response(contents);
    config->templates->status_400_template = malloc(strlen(contents) + 1);
    memcpy(config->templates->status_400_template, contents, strlen(contents));
    config->templates->status_400_template[strlen(contents)] = '\0';
  } else {
    char *contents = read_file(status_400_template->valuestring);
    char *header = malloc(205 * sizeof(char));
    sprintf(header,
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Length: %ld\r\n"
            "Content-Type: text/html\r\n"
            "Connection: Close\r\n"
            "\r\n",
            strlen(contents));
    config->templates->status_400_template = malloc(205 + strlen(contents) + 1);
    memcpy(config->templates->status_400_template, header, strlen(header));
    strcat(config->templates->status_400_template, contents);
  }

  status_404_template =
      cJSON_GetObjectItemCaseSensitive(templates, "status_404_template");
  if (!cJSON_IsString(status_404_template) ||
      !status_404_template->valuestring ||
      strcmp(status_404_template->valuestring, "") == 0) {
    char *contents = malloc(4096 * sizeof(char));
    http_404_response(contents);
    config->templates->status_404_template = malloc(strlen(contents) + 1);
    memcpy(config->templates->status_404_template, contents, strlen(contents));
    config->templates->status_404_template[strlen(contents)] = '\0';
  } else {
    char *contents = read_file(status_404_template->valuestring);
    char *header = malloc(200 * sizeof(char));
    sprintf(header,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: %ld\r\n"
            "Content-Type: text/html\r\n"
            "Connection: Close\r\n"
            "\r\n",
            strlen(contents));
    config->templates->status_404_template = malloc(200 + strlen(contents) + 1);
    memcpy(config->templates->status_404_template, header, strlen(header));
    strcat(config->templates->status_404_template, contents);
  }

  status_502_template =
      cJSON_GetObjectItemCaseSensitive(templates, "status_502_template");
  if (!cJSON_IsString(status_502_template) ||
      !status_502_template->valuestring ||
      strcmp(status_502_template->valuestring, "") == 0) {
    char *contents = malloc(4096 * sizeof(char));
    http_502_response(contents);
    config->templates->status_502_template = malloc(strlen(contents) + 1);
    memcpy(config->templates->status_502_template, contents, strlen(contents));
    config->templates->status_502_template[strlen(contents)] = '\0';
  } else {
    char *contents = read_file(status_502_template->valuestring);
    char *header = malloc(205 * sizeof(char));
    sprintf(header,
            "HTTP/1.1 502 Bad Gateway\r\n"
            "Content-Length: %ld\r\n"
            "Content-Type: text/html\r\n"
            "Connection: Close\r\n"
            "\r\n",
            strlen(contents));
    config->templates->status_502_template = malloc(205 + strlen(contents) + 1);
    memcpy(config->templates->status_502_template, header, strlen(header));
    strcat(config->templates->status_502_template, contents);
  }

  config->num_proxies = 0;
  proxies = cJSON_GetObjectItemCaseSensitive(json, "proxies");
  cJSON_ArrayForEach(proxy, proxies) {
    config->num_proxies++;

    config->proxies[config->num_proxies - 1] = malloc(sizeof(proxy_config_t));
    proxy_config_t *proxy_config = config->proxies[config->num_proxies - 1];
    memset(proxy_config, 0, sizeof(proxy_config_t));

    proxy_hosts = cJSON_GetObjectItemCaseSensitive(proxy, "hosts");
    proxy_config->num_hosts = 0;
    cJSON_ArrayForEach(proxy_host, proxy_hosts) {
      if (cJSON_IsString(proxy_host) && proxy_host->valuestring) {
        proxy_config->num_hosts++;
        proxy_config->hosts[proxy_config->num_hosts - 1] =
            malloc(strlen(proxy_host->valuestring) + 1);
        memcpy(proxy_config->hosts[proxy_config->num_hosts - 1],
               proxy_host->valuestring, strlen(proxy_host->valuestring));
        proxy_config->hosts[proxy_config->num_hosts - 1]
                           [strlen(proxy_host->valuestring)] = '\0';
      }
    }

    proxy_ip = cJSON_GetObjectItemCaseSensitive(proxy, "ip");
    if (cJSON_IsString(proxy_ip) && proxy_ip->valuestring) {
      proxy_config->ip = malloc(20);
      memcpy(proxy_config->ip, proxy_ip->valuestring,
             strlen(proxy_ip->valuestring));
      proxy_config->ip[strlen(proxy_ip->valuestring)] = '\0';
    }

    proxy_port = cJSON_GetObjectItemCaseSensitive(proxy, "port");
    if (cJSON_IsNumber(proxy_port) && proxy_port->valueint) {
      proxy_config->port = proxy_port->valueint;
    }

    bool ssl_enabled = config->secure_port > 0;

    certificate_path =
        cJSON_GetObjectItemCaseSensitive(proxy, "certificate_path");
    if (!cJSON_IsString(certificate_path) || !certificate_path->valuestring) {
      ssl_enabled = false;
    }

    key_path = cJSON_GetObjectItemCaseSensitive(proxy, "key_path");
    if (!cJSON_IsString(key_path) || !key_path->valuestring) {
      ssl_enabled = false;
    }

    if (ssl_enabled) {
      proxy_config->ssl_context = SSL_CTX_new(SSLv23_method());
      if (!SSL_CTX_use_certificate_chain_file(proxy_config->ssl_context,
                                              certificate_path->valuestring)) {
        int err = ERR_get_error();
        log_error("Could not load certificate file: %s; reason: %s",
                  certificate_path->valuestring, ERR_error_string(err, NULL));
        ssl_enabled = false;
      }
      if (ssl_enabled && !SSL_CTX_use_PrivateKey_file(proxy_config->ssl_context,
                                                      key_path->valuestring,
                                                      SSL_FILETYPE_PEM)) {
        int err = ERR_get_error();
        log_error(
            "Could not load key file: %s or key doesn't match certificate: %s; "
            "reason: %s",
            key_path->valuestring, certificate_path->valuestring,
            ERR_error_string(err, NULL));
        ssl_enabled = false;
      }
    }
    if (ssl_enabled) {
      if (uv_ssl_setup_recommended_secure_context(proxy_config->ssl_context)) {
        log_error("configuring recommended secure context");
        ssl_enabled = false;
      }
    }

    force_ssl = cJSON_GetObjectItemCaseSensitive(proxy, "force_ssl");
    if (cJSON_IsBool(force_ssl)) {
      proxy_config->force_ssl = force_ssl->type == cJSON_True;
    }

    ssl_passthrough =
        cJSON_GetObjectItemCaseSensitive(proxy, "ssl_passthrough");
    if (cJSON_IsBool(ssl_passthrough)) {
      if (ssl_enabled) {
        log_warn(
            "ssl_passthrough enabled, certificate and key file will be "
            "ignored!");
        ssl_enabled = false;
      }
      proxy_config->ssl_passthrough = ssl_passthrough->type == cJSON_True;
      proxy_config->force_ssl =
          proxy_config->ssl_passthrough ? true : proxy_config->force_ssl;
    }

    if (!ssl_enabled) {
      SSL_CTX_free(proxy_config->ssl_context);
      proxy_config->ssl_context = NULL;
      force_ssl = false;
    }
  }

  cJSON_Delete(json);
}

void http_400_response(char *resp) {
  snprintf(resp, 1024,
           "HTTP/1.1 400 Bad Request\r\n"
           "Content-Length: %ld\r\n"
           "Content-Type: text/html\r\n"
           "Connection: Close\r\n"
           "\r\n"
           "<html>\r\n"
           "<head>\r\n"
           "<title>400 Bad Request</title>\r\n"
           "</head>\r\n"
           "<body>\r\n"
           "<h1 align=\"center\">400 Bad Request</h1>\r\n"
           "<hr/>\r\n"
           "<p align=\"center\">bproxy %s</p>\r\n"
           "</body>\r\n"
           "</html>\r\n",
           162 + strlen(VERSION), VERSION);
}

void http_404_response(char *resp) {
  snprintf(
      resp, 4096,
      "HTTP/1.1 404 Not Found\r\n"
      "Content-Length: %ld\r\n"
      "Content-Type: text/html\r\n"
      "Connection: Close\r\n"
      "\r\n"
      "<html><head><title>404 Not Found</title> <style type=\"text/css\">html, "
      "body { background: #F5F5F5; color: #4D5152; font-family: Verdana, "
      "Geneva, Tahoma, sans-serif; } .illustration { margin: 50px auto; width: "
      "286px; } .txt, .version { font-size: 14px; display: block; text-align: "
      "center; } .txt { font-size: 22px; } a { color: #4D5152; "
      "}</style></head><body><div class=\"illustration\"><svg "
      "xmlns=\"http://www.w3.org/2000/svg\" width=\"286\" height=\"276\" "
      "viewBox=\"0 0 286 276\"><g fill=\"none\" fill-rule=\"evenodd\"><polygon "
      "fill=\"#FFF\" points=\"4.931 4.929 281.069 4.929 281.069 271.071 4.931 "
      "271.071\" /><polygon fill=\"#3FADD5\" points=\"19.724 59.143 266.276 "
      "59.143 266.276 256.286 19.724 256.286\" /><path fill=\"#4D5152\" "
      "d=\"M0,0 L286,0 L286,276 L0,276 L0,0 Z M9.86206897,266.142857 "
      "L276.137931,266.142857 L276.137931,9.85714286 L9.86206897,9.85714286 "
      "L9.86206897,266.142857 Z\" /><polygon fill=\"#4D5152\" points=\"9.862 "
      "39.429 276.138 39.429 276.138 49.286 9.862 49.286\" /><polygon "
      "fill=\"#4D5152\" points=\"19.724 19.714 29.586 19.714 29.586 29.571 "
      "19.724 29.571\" /><polygon fill=\"#4D5152\" points=\"39.448 19.714 "
      "49.31 19.714 49.31 29.571 39.448 29.571\" /><polygon fill=\"#4D5152\" "
      "points=\"59.172 19.714 69.034 19.714 69.034 29.571 59.172 29.571\" "
      "/><polygon fill=\"#4D5152\" points=\"78.897 9.857 88.759 9.857 88.759 "
      "39.429 78.897 39.429\" /><polygon fill=\"#FFF\" points=\"19.724 108.429 "
      "266.276 108.429 266.276 207 19.724 207\" /><polygon fill=\"#4D5152\" "
      "points=\"98.621 19.714 266.276 19.714 266.276 29.571 98.621 29.571\" "
      "/><path fill=\"#4D5152\" d=\"M147.931034,197.142857 "
      "L138.068966,197.142857 C127.191103,197.142857 118.344828,188.301 "
      "118.344828,177.428571 L118.344828,138 C118.344828,127.127571 "
      "127.191103,118.285714 138.068966,118.285714 L147.931034,118.285714 "
      "C158.808897,118.285714 167.655172,127.127571 167.655172,138 "
      "L167.655172,177.428571 C167.655172,188.301 158.808897,197.142857 "
      "147.931034,197.142857 L147.931034,197.142857 Z M138.068966,128.142857 "
      "C132.630034,128.142857 128.206897,132.568714 128.206897,138 "
      "L128.206897,177.428571 C128.206897,182.864786 132.630034,187.285714 "
      "138.068966,187.285714 L147.931034,187.285714 C153.369966,187.285714 "
      "157.793103,182.864786 157.793103,177.428571 L157.793103,138 "
      "C157.793103,132.568714 153.369966,128.142857 147.931034,128.142857 "
      "L138.068966,128.142857 L138.068966,128.142857 Z\" /><polygon "
      "fill=\"#4D5152\" points=\"98.621 118.286 108.483 118.286 108.483 "
      "197.143 98.621 197.143\" /><polyline fill=\"#4D5152\" points=\"103.552 "
      "167.571 71.924 167.571 64.103 159.755 64.103 118.286 73.966 118.286 "
      "73.966 155.674 76.007 157.714 103.552 157.714 103.552 167.571\" "
      "/><polygon fill=\"#4D5152\" points=\"212.034 118.286 221.897 118.286 "
      "221.897 197.143 212.034 197.143\" /><polyline fill=\"#4D5152\" "
      "points=\"216.966 167.571 185.338 167.571 177.517 159.755 177.517 "
      "118.286 187.379 118.286 187.379 155.674 189.421 157.714 216.966 157.714 "
      "216.966 167.571\" /><polygon fill=\"#3290B3\" points=\"157.793 78.857 "
      "256.414 78.857 256.414 88.714 157.793 88.714\" /><polygon "
      "fill=\"#3290B3\" points=\"29.586 78.857 128.207 78.857 128.207 88.714 "
      "29.586 88.714\" /><polygon fill=\"#3290B3\" points=\"138.069 78.857 "
      "147.931 78.857 147.931 88.714 138.069 88.714\" /><polygon "
      "fill=\"#3290B3\" points=\"157.793 226.714 256.414 226.714 256.414 "
      "236.571 157.793 236.571\" /><polygon fill=\"#3290B3\" points=\"29.586 "
      "226.714 128.207 226.714 128.207 236.571 29.586 236.571\" /><polygon "
      "fill=\"#3290B3\" points=\"138.069 226.714 147.931 226.714 147.931 "
      "236.571 138.069 236.571\" /></g></svg></div><p class=\"txt\">Page not "
      "found.</p><p class=\"version\"><a "
      "href=\"https://github.com/bleenco/bproxy\">bproxy</a> "
      "v%s</p></body></html>\r\n",
      3577 + strlen(VERSION), VERSION);
}

void http_502_response(char *resp) {
  snprintf(resp, 1024,
           "HTTP/1.1 502 Bad Gateway\r\n"
           "Content-Length: %ld\r\n"
           "Content-Type: text/html\r\n"
           "Connection: Close\r\n"
           "\r\n"
           "<html>\r\n"
           "<head>\r\n"
           "<title>502 Bad Gateway</title>\r\n"
           "</head>\r\n"
           "<body>\r\n"
           "<h1 align=\"center\">502 Bad Gateway</h1>\r\n"
           "<hr/>\r\n"
           "<p align=\"center\">bproxy %s</p>\r\n"
           "</body>\r\n"
           "</html>\r\n",
           160 + strlen(VERSION), VERSION);
}
