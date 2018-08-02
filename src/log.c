/**
 * @license
 * Copyright Bleenco GmbH. All Rights Reserved.
 *
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file at https://github.com/bleenco/bproxy
 */
#include "log.h"

// clang-format off
static const char *level_names[] =
{
  "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char *level_colors[] =
{
  "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
// clang-format on

void log_set_fp(FILE *fp) { L.fp = fp; }

void log_set_level(int level) { L.level = level; }

void log_set_quiet(int enable) { L.quiet = enable ? 1 : 0; }

void log_log(int level, const char *fmt, ...) {
  if (level < L.level) {
    return;
  }

  time_t t = time(NULL);
  struct tm *lt = localtime(&t);

  // log to stderr
  if (!L.quiet) {
    va_list args;
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';
    fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m \x1b[0m ", buf,
            level_colors[level], level_names[level]);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
  }

  // log to file
  if (L.fp) {
    va_list args;
    char buf[32];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';
    fprintf(L.fp, "%s %-5s ", buf, level_names[level]);
    va_start(args, fmt);
    vfprintf(L.fp, fmt, args);
    va_end(args);
    fprintf(L.fp, "\n");
    fflush(L.fp);
  }
}
