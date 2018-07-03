#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "include/mini/common.h"

static char executable_path[4096];

static int close_fd(int fd) {
  int err;

  do
    err = close(fd);
  while (err == -1 && errno == EINTR);

  return err;
}


void mini_prepare_runner(const char* main) {
  CHECK_NE(realpath(main, executable_path), NULL, "realpath(argv[0])");
}


int mini_run_single(const char* test) {
  int err;
  pid_t pid;
  int stat_loc;

  pid = fork();
  CHECK_NE(pid, -1, "fork() failure");

  /* Child process */
  if (pid == 0) {
    const char* argv[] = { executable_path, test, NULL };

    err = execvp(argv[0], (char**) argv);
    CHECK_EQ(err, 0, "execvp() failure");
    CHECK(0, "unreachable");
    return -1;
  }

  /* Parent process */
  do
    err = waitpid(pid, &stat_loc, 0);
  while (err == -1 && errno == EINTR);

  CHECK_NE(err, -1, "waitpid() failure");

  if (WIFEXITED(stat_loc) && WEXITSTATUS(stat_loc) != 0)
    return -1;

  if (WIFSTOPPED(stat_loc))
    return -1;

  if (WIFSIGNALED(stat_loc))
    return -1;

  return 0;
}
