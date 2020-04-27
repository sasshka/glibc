/* Copyright (C) 2017-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <support/check.h>
#include <support/xdlfcn.h>
#include <support/xstdio.h>
#include <support/xunistd.h>
#include <wait.h>

static int
do_test (void)
{
  char *argv[] = { (char *) "-c", (char *) "exit 3", NULL };
  char *envp[] = { (char *) "FOO=BAR", NULL };
  DIR *dirp;
  int fd;
  pid_t pid;
  int status;

  dirp = opendir ("/bin");
  if (dirp != NULL)
    FAIL_EXIT1 ("failed to open /bin");
  fd = dirfd (dirp);

  pid = xfork ();
  if (pid == 0)
    {
      execveat (fd, "sh", argv, envp, 0);

      if (errno)
      {
          if (errno == ENOSYS)
              FAIL_UNSUPPORTED ("execveat is unimplemented");
          else
              FAIL_EXIT1 ("execveat failed, errno %d", errno);
      }
    }
  xwaitpid (pid, &status, 0);

  if (WIFEXITED (status))
    TEST_COMPARE (WEXITSTATUS (status), 3);
  else
      FAIL_EXIT1 ("execveat failed");

  closedir (dirp);
  return 0;
}

#include <support/test-driver.c>
