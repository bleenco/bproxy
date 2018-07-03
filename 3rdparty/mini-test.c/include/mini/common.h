#ifndef INCLUDE_MINI_COMMON_H_
#define INCLUDE_MINI_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <Windows.h>
#define ABORT ExitProcess(42)
#else
#define ABORT abort()
#endif

#define CHECK(VALUE, MESSAGE)                                                \
    do {                                                                     \
      if ((VALUE)) break;                                                    \
      fprintf(stderr, "%s[%d] Assertion failure: " #MESSAGE "\n",            \
              __FILE__, __LINE__);                                           \
      ABORT;                                                                 \
    } while (0)

#define CHECK_EQ(A, B, MESSAGE) CHECK((A) == (B), MESSAGE)
#define CHECK_NE(A, B, MESSAGE) CHECK((A) != (B), MESSAGE)

void mini_prepare_runner(const char* main);
int mini_run_single(const char* test);

#endif  /* INCLUDE_MINI_COMMON_H_ */
