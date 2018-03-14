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
      req->host = malloc((nob + 1) * sizeof(char));
      memcpy(req->host, req->headers[i][1], nob);
      req->host[nob] = '\0';
      parse_requested_host(req);
    }
    else if (strcasecmp(req->headers[i][0], "Accept-Encoding") == 0)
    {
      if (strcasecmp(req->headers[i][1], "gzip, deflate") == 0)
      {
        req->enable_compression = true;
      }
    }
  }

  return 0;
}

int body_cb(http_parser *p, const char *buf, size_t length)
{
  p->data = malloc(length + 1);
  strncpy(p->data, buf, length);

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

    request->hostname = malloc(len + 1);
    memcpy(request->hostname, host, len);
    request->hostname[len] = '\0';
  }
  else
  {
    int nob = strlen(host);
    request->hostname = malloc(nob + 1);
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
           "Content-Length: 160\r\n"
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
