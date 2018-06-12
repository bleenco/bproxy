/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/bproxy
 */
#include "http.h"

int message_begin_cb(http_parser *p)
{
  http_link_context_t *context = p->data;
  http_request_t *request = &context->request;
  for (int i = 0; i < MAX_HEADERS; i++)
  {
    request->headers[i][0][0] = 0;
    request->headers[i][1][0] = 0;
  }
  request->num_headers = 0;
  request->last_header_element = NONE;
  request->upgrade = 0;
  request->keepalive = 0;
  return 0;
}

int url_cb(http_parser *p, const char *buf, size_t len)
{
  http_link_context_t *context = p->data;
  http_request_t *request = &context->request;
  free(request->url);
  request->url = malloc((len + 1) * sizeof(char));
  memcpy(request->url, buf, len);
  request->url[len] = '\0';
  return 0;
}

int headers_field_cb(http_parser *p, const char *buf, size_t len)
{
  http_link_context_t *context = p->data;
  http_request_t *request = &context->request;
  if (request->last_header_element != FIELD)
  {
    request->num_headers++;
  }
  strncat(request->headers[request->num_headers - 1][0], buf, len);
  request->last_header_element = FIELD;
  return 0;
}

int headers_value_cb(http_parser *p, const char *buf, size_t len)
{
  http_link_context_t *context = p->data;
  http_request_t *request = &context->request;
  strncat(request->headers[request->num_headers - 1][1], buf, len);
  request->last_header_element = VALUE;
  return 0;
}

int headers_complete_cb(http_parser *p)
{
  http_link_context_t *context = p->data;
  http_request_t *request = &context->request;
  request->keepalive = http_should_keep_alive(p);
  request->http_major = p->http_major;
  request->http_minor = p->http_minor;
  request->method = p->method;
  request->upgrade = p->upgrade;
  request->content_length = p->content_length;

  request->enable_compression = false;

  for (int i = 0; i < request->num_headers; i++)
  {
    if (strcasecmp(request->headers[i][0], "Host") == 0)
    {
      int nob = strlen(request->headers[i][1]);
      memcpy(request->host, request->headers[i][1], nob);
      request->host[nob] = '\0';
      parse_requested_host(request);
    }
    else if (strcasecmp(request->headers[i][0], "Accept-Encoding") == 0)
    {
      if (strstr(request->headers[i][1], "gzip"))
      {
        request->enable_compression = true;
      }
    }
  }

  // Proxy headers
  char *proto = context->https ? "https" : "http";

  strcpy(request->headers[request->num_headers][0], "X-Forwarded-For");
  strcpy(request->headers[request->num_headers++][1], context->peer_ip);

  strcpy(request->headers[request->num_headers][0], "X-Forwarded-Host");
  strcpy(request->headers[request->num_headers++][1], request->host);

  strcpy(request->headers[request->num_headers][0], "X-Forwarded-Proto");
  strcpy(request->headers[request->num_headers++][1], proto);

  return 0;
}

int body_cb(http_parser *p, const char *buf, size_t length)
{
  return 0;
}

int message_complete_cb(http_parser *p)
{
  http_link_context_t *context = p->data;
  context->request.complete = true;
  return 0;
}

void parse_requested_host(http_request_t *request)
{
  char *host = request->host;
  if (strstr(host, ":"))
  {
    char *e = strchr(host, ':');
    int index = (int)(e - host);
    int len = strlen(host);
    host[len - (len - index)] = '\0';
    len = strlen(host);

    memcpy(request->hostname, host, len);
    request->hostname[len] = '\0';
  }
  else
  {
    int nob = strlen(host);
    memcpy(request->hostname, host, nob);
    request->hostname[nob] = '\0';
  }
}

int insert_header(char *src, char *resp)
{
  const char *find = strstr(resp, "\r\n\r\n") + 3;
  if (find)
  {
    int pos = (int)find - (int)resp;
    insert_substring(resp, src, pos);
    return 1;
  }

  return 0;
}

void insert_substring(char *a, char *b, int position)
{
  char *f;
  int length;

  length = strlen(a);

  f = substring(a, 1, position - 1);

  strcpy(a, "");
  strcat(a, f);
  free(f);
  strcat(a, b);
}

char *substring(char *string, int position, int length)
{
  char *pointer;
  int c;

  pointer = malloc(length + 1);

  if (pointer == NULL)
    exit(EXIT_FAILURE);

  for (c = 0; c < length; c++)
    *(pointer + c) = *((string + position - 1) + c);

  *(pointer + c) = '\0';

  return pointer;
}

void http_400_response(char *resp)
{
  snprintf(resp, 1024,
           "HTTP/1.1 400 Bad Request\r\n"
           "Content-Length: 165\r\n"
           "Content-Type: text/html\r\n"
           "Connection: Close\r\n"
           "Server: vex/%s\r\n"
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
           VERSION, VERSION);
}

void http_404_response(char *resp)
{
  snprintf(resp, 1024,
           "HTTP/1.1 404 Not Found\r\n"
           "Content-Length: 161\r\n"
           "Content-Type: text/html\r\n"
           "Connection: Close\r\n"
           "Server: vex/%s\r\n"
           "\r\n"
           "<html>\r\n"
           "<head>\r\n"
           "<title>404 Not Found</title>\r\n"
           "</head>\r\n"
           "<body>\r\n"
           "<h1 align=\"center\">404 Not Found</h1>\r\n"
           "<hr/>\r\n"
           "<p align=\"center\">bproxy %s</p>\r\n"
           "</body>\r\n"
           "</html>\r\n",
           VERSION, VERSION);
}

void http_502_response(char *resp)
{
  snprintf(resp, 1024,
           "HTTP/1.1 502 Bad Gateway\r\n"
           "Content-Length: 167\r\n"
           "Content-Type: text/html\r\n"
           "Connection: Close\r\n"
           "Server: vex/%s\r\n"
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
           VERSION, VERSION);
}

int response_message_begin_cb(http_parser *p)
{
  http_link_context_t *context = p->data;
  http_response_t *response = &context->response;
  for (int i = 0; i < MAX_HEADERS; i++)
  {
    response->headers[i][0][0] = 0;
    response->headers[i][1][0] = 0;
  }
  response->num_headers = 0;
  response->last_header_element = NONE;
  response->expected_data_len = 0;
  response->processed_data_len = 0;
  return 0;
}

int response_headers_field_cb(http_parser *p, const char *buf, size_t len)
{
  http_link_context_t *context = p->data;
  http_response_t *response = &context->response;
  if (response->last_header_element != FIELD)
  {
    response->num_headers++;
  }
  strncat(response->headers[response->num_headers - 1][0], buf, len);
  response->last_header_element = FIELD;
  return 0;
}

int response_headers_value_cb(http_parser *p, const char *buf, size_t len)
{
  http_link_context_t *context = p->data;
  http_response_t *response = &context->response;
  strncat(response->headers[response->num_headers - 1][1], buf, len);
  response->last_header_element = VALUE;
  return 0;
}

int response_headers_complete_cb(http_parser *p)
{
  http_link_context_t *context = p->data;
  http_response_t *response = &context->response;
  context->response.enable_compression = false;
  boolean already_compressed = false;

  for (int i = 0; i < response->num_headers; i++)
  {
    if (strcasecmp(response->headers[i][0], "Content-Type") == 0)
    {
      for (int j = 0; j < context->server_config->num_gzip_mime_types; j++)
      {
        if (strstr(response->headers[i][1], context->server_config->gzip_mime_types[j]))
        {
          response->enable_compression = true;
          continue;
        }
      }
    }
    else if (strcasecmp(response->headers[i][0], "Content-Encoding") == 0)
    {
      already_compressed = true;
    }
    else if (strcasecmp(response->headers[i][0], "Content-Length") == 0)
    {
      response->expected_data_len = atoi(response->headers[i][1]);
    }
  }

  if (already_compressed)
  {
    response->enable_compression = false;
  }
  return 0;
}

void http_init_response_headers(http_response_t *response, bool compressed)
{
  memset(response->http_header, 0, sizeof response->http_header);
  strcat(response->http_header, response->status_line);
  strcat(response->http_header, "\r\n");
  for (int i = 0; i < response->num_headers; ++i)
  {
    // Skip responses of non compressed headers
    if (compressed && (strcasecmp(response->headers[i][0], "Content-Length") == 0 || strcasecmp(response->headers[i][0], "Accept-Ranges") == 0))
    {
      continue;
    }

    strcat(response->http_header, response->headers[i][0]);
    strcat(response->http_header, ": ");
    strcat(response->http_header, response->headers[i][1]);
    strcat(response->http_header, "\r\n");
  }
  if (compressed)
  {
    // Add gzip headers
    strcat(response->http_header, "Transfer-Encoding: chunked\r\n");
    strcat(response->http_header, "Content-Encoding: gzip\r\n");
  }
  char name_and_version[100];
  sprintf(name_and_version, "Via: bproxy %s\r\n\r\n", VERSION);
  strcat(response->http_header, name_and_version);
  response->http_header_len = strlen(response->http_header);
}

void http_init_request_headers(http_link_context_t *context)
{
  http_request_t *request = &context->request;
  memset(request->http_header, 0, sizeof request->http_header);
  strcat(request->http_header, request->status_line);
  strcat(request->http_header, "\r\n");
  for (int i = 0; i < request->num_headers; ++i)
  {
    strcat(request->http_header, request->headers[i][0]);
    strcat(request->http_header, ": ");
    strcat(request->http_header, request->headers[i][1]);
    strcat(request->http_header, "\r\n");
  }

  strcat(request->http_header, "\r\n");
  request->http_header_len = strlen(request->http_header);
}
