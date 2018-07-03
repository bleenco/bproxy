#define TEST_LIST(V) \
    V(first) \
    V(second)

#include "mini/main.h"

TEST_IMPL(first) {
  CHECK_EQ(0, 1, "0 should be equal to 1");
}

TEST_IMPL(second) {
  CHECK_EQ(0, 0, "0 should be equal to 1");
}
