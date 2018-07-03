#ifndef INCLUDE_MINI_MAIN_H_
#define INCLUDE_MINI_MAIN_H_

#include "mini/test.h"

#define TEST_COUNT(N) +1

#define TEST_TAP(N)                                                           \
    do {                                                                      \
      int err;                                                                \
      err = mini_run_single(#N);                                              \
      if (err != 0)                                                           \
        errors++;                                                             \
      fprintf(stdout, "%s - %s\n", err == 0 ? "ok" : "not ok", #N);           \
    } while (0)


#define TEST_SELECT(N)                                                        \
    if (strncmp(argv[1], #N, sizeof(#N) - 1) == 0) {                          \
      TEST_FN(N)();                                                           \
      return 0;                                                               \
    }

#define TEST_RUN(N)                                                           \
    do {                                                                      \
      TEST_TAP(N);                                                            \
    } while (0);

int main(int argc, char** argv) {
  int errors;

  if (argc == 2) {
    TEST_ENUM(TEST_SELECT)
    return -1;
  }

  mini_prepare_runner(argv[0]);

  errors = 0;
  fprintf(stdout, "1..%d\n", 0 TEST_ENUM(TEST_COUNT));
  TEST_ENUM(TEST_RUN)
  fflush(stdout);
  fflush(stderr);

  return errors == 0 ? 0 : -1;
}

#undef TEST_RUN
#undef TEST_SELECT
#undef TEST_COUNT

#endif  /* INCLUDE_MINI_MAIN_H_ */
