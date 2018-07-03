#include "test-common.h"

static int write_cb_called;
static int test_arg;

static void write_client() {
  char buf[1024];

  CHECK_EQ(SSL_read(client.ssl, buf, sizeof(buf)), 5, "SSL_read() == 5");
  CHECK_EQ(strncmp(buf, "hello", 5), 0, "write_client data match");

  CHECK_EQ(SSL_read(client.ssl, buf, sizeof(buf)), 5, "SSL_read() == 5");
  CHECK_EQ(strncmp(buf, "world", 5), 0, "write_client data match");
}


static void write_cb(uv_link_t* link, int status, void* arg) {
  CHECK_EQ(status, 0, "write_cb() status");
  CHECK_EQ(link, (uv_link_t*) &server.observer,
           "write_cb() link must be server");
  CHECK_EQ(arg, &test_arg, "write_cb() link arg be &test_arg");

  write_cb_called++;

  CHECK_EQ(uv_link_read_stop((uv_link_t*) &server.observer), 0,
           "uv_link_read_stop(server)");

  if (write_cb_called == 1) {
    uv_buf_t buf;

    buf = uv_buf_init("world", 5);
    CHECK_EQ(uv_link_write(link, &buf, 1, NULL, write_cb, &test_arg), 0,
             "uv_link_write(server)");
  }
}


static void write_info_cb(const SSL* ssl, int where, int val) {
  if ((where & SSL_CB_HANDSHAKE_DONE) != 0) {
    CHECK_EQ(uv_link_read_stop((uv_link_t*) &server.observer), 0,
             "uv_link_read_stop(server)");
  }
}


static void write_server() {
  uv_link_t* serv;
  uv_buf_t buf;

  serv = (uv_link_t*) &server.observer;
  SSL_set_info_callback(server.ssl, write_info_cb);
  CHECK_EQ(uv_run(loop, UV_RUN_DEFAULT), 0, "uv_run()");

  buf = uv_buf_init("hello", 5);
  CHECK_EQ(uv_link_write(serv, &buf, 1, NULL, write_cb, &test_arg), 0,
           "uv_link_write(server)");

  CHECK_EQ(uv_run(loop, UV_RUN_DEFAULT), 0, "uv_run()");
  CHECK_EQ(write_cb_called, 2, "number of write_cb's");
}


TEST_IMPL(write) {
  ssl_client_server_test(write_client, write_server);
}
