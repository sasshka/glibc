/* Test ldconfig does not segfault when aux-cache is corrupted (Bug 18093).
   Copyright (C) 2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, see <http://www.gnu.org/licenses/>.  */

/* This test does the following:
   Run ldconfig to create the caches.
   Corrupt the caches.
   Run ldconfig again.
   At each step we verify that ldconfig does not crash.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <ftw.h>
#include <stdint.h>

#include <support/capture_subprocess.h>
#include <support/check.h>
#include <support/support.h>
#include <support/xunistd.h>

#include <dirent.h>

/* Run ldconfig with a corrupt aux-cache, in particular we test for size
   truncation that might happen if a previous ldconfig run failed or if
   there were storage or power issues while we were writing the file.
   We want ldconfig not to crash, and it should be able to do so by
   computing the expected size of the file (bug 18093).  */
static void
do_test_ldconfig (void *closure)
{
  char *prog = xasprintf ("%s/ldconfig", support_install_rootsbindir);
  char *const args[] = { prog, NULL };
  const char *path = "/var/cache/ldconfig/aux-cache";
  struct stat64 fs;
  long int size, new_size, i;
  int status;
  pid_t pid;

  /* Create the needed directories. */
  xmkdirp ("/var/cache/ldconfig", 0777);

  pid = xfork ();
  /* Run ldconfig fist to generate the aux-cache.  */
  if (pid == 0)
    {
      execv (args[0], args);
      _exit (1);
    }
  else
    {
      xwaitpid (pid, &status, 0);
      TEST_COMPARE(status, 0);
      xstat (path, &fs);

      size = fs.st_size;
      /* Run 3 tests, each truncating aux-cache shorter and shorter.  */
      for (i = 3; i > 0; i--)
        {
          new_size = size * i / 4;
          if (truncate (path, new_size))
              FAIL_EXIT1 ("truncation failed: %m");

          pid = xfork ();
          /* Verify that ldconfig can run with a truncated
             aux-cache and doesn't crash.  */
          if (pid == 0)
            {
              execv (args[0], args);
              _exit (1);
            }
          else
            {
              xwaitpid (pid, &status, 0);
              TEST_COMPARE(status, 0);
            }
        }
    }

  free (prog);
}

static int
do_test (void)
{

  struct support_capture_subprocess result;
  result = support_capture_subprocess (do_test_ldconfig, NULL);
  support_capture_subprocess_check (&result, "ldconfig", 0, sc_allow_none);

  return 0;
}

#include <support/test-driver.c>
