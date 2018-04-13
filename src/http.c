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
  conn_t *conn = p->data;
  for (int i = 0; i < MAX_HEADERS; i++)
  {
    conn->request->headers[i][0][0] = 0;
    conn->request->headers[i][1][0] = 0;
  }
  conn->request->num_headers = 0;
  conn->request->last_header_element = NONE;
  conn->request->upgrade = 0;
  conn->request->keepalive = 0;
  return 0;
}

int url_cb(http_parser *p, const char *buf, size_t len)
{
  conn_t *conn = p->data;
  free(conn->request->url);
  conn->request->url = malloc((len + 1) * sizeof(char));
  memcpy(conn->request->url, buf, len);
  conn->request->url[len] = '\0';
  return 0;
}

int headers_field_cb(http_parser *p, const char *buf, size_t len)
{
  conn_t *conn = p->data;
  http_request_t *req = conn->request;
  if (req->last_header_element != FIELD)
  {
    req->num_headers++;
  }
  strncat(req->headers[req->num_headers - 1][0], buf, len);
  req->last_header_element = FIELD;
  return 0;
}

int headers_value_cb(http_parser *p, const char *buf, size_t len)
{
  conn_t *conn = p->data;
  http_request_t *req = conn->request;
  strncat(req->headers[req->num_headers - 1][1], buf, len);
  req->last_header_element = VALUE;
  return 0;
}

int headers_complete_cb(http_parser *p)
{
  conn_t *conn = p->data;
  http_request_t *req = conn->request;
  req->keepalive = http_should_keep_alive(p);
  req->http_major = p->http_major;
  req->http_minor = p->http_minor;
  req->method = p->method;
  req->upgrade = p->upgrade;

  req->enable_compression = false;

  for (int i = 0; i < req->num_headers; i++)
  {
    if (strcasecmp(req->headers[i][0], "Host") == 0)
    {
      int nob = strlen(req->headers[i][1]);
      memcpy(req->host, req->headers[i][1], nob);
      req->host[nob] = '\0';
      parse_requested_host(req);
    }
    else if (strcasecmp(req->headers[i][0], "Accept-Encoding") == 0)
    {
      if (strstr(req->headers[i][1], "gzip"))
      {
        req->enable_compression = true;
      }
    }
  }

  return 0;
}

int body_cb(http_parser *p, const char *buf, size_t length)
{
  http_response_t *response = p->data;
  free(response->raw_body);
  response->raw_body = malloc(length);
  strncpy(response->raw_body, buf, length);
  response->body_size = length;

  return 0;
}

int message_complete_cb(http_parser *p)
{
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

int response_message_begin_cb(http_parser *p)
{
  http_response_t *response = p->data;
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
  http_response_t *response = p->data;
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
  http_response_t *response = p->data;
  strncat(response->headers[response->num_headers - 1][1], buf, len);
  response->last_header_element = VALUE;
  return 0;
}

int response_headers_complete_cb(http_parser *p)
{
  http_response_t *response = p->data;
  response->enable_compression = false;
  boolean already_compressed = false;

  for (int i = 0; i < response->num_headers; i++)
  {
    if (strcasecmp(response->headers[i][0], "Content-Type") == 0)
    {
      for (int j = 0; j < response->server_config->num_gzip_mime_types; j++)
      {
        if (strstr(response->headers[i][1], response->server_config->gzip_mime_types[j]))
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
