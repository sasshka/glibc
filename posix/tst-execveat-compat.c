/* Test the fallback implementation of execveat.
   Copyright (C) 2021 Free Software Foundation, Inc.
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
   <http://www.gnu.org/licenses/>.  */

/* Get the declaration of the official execveat function.  */
#include <unistd.h>

/* Compile a local version of execveat.  */
#include <sysdeps/unix/sysv/linux/execveat_fallback.c>

int
__execveat_fallback (int dirfd, const char *path, char *const argv[],
                     char *const envp[], int flags);

/* Re-use the test, but run it against copy_file_range_compat defined
   above.  */
#define execveat execveat_fallback
#include "tst-execveat.c"
