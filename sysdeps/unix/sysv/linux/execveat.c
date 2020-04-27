/* Execute program relative to a directory file descriptor.
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
   <https://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <sysdep.h>
#include <sys/syscall.h>
#include <kernel-features.h>
#include <fd_to_filename.h>
#include <not-cancel.h>
#ifndef __ASSUME_EXECVEAT
# include "execveat_fallback.c"
#endif

/* Execute the file FD refers to, overlaying the running program image.
   ARGV and ENVP are passed to the new program, as for `execve'.  */
int
execveat (int dirfd, const char *path, char *const argv[], char *const envp[],
          int flags)
{
  /* Avoid implicit array coercion in syscall macros.  */
    INLINE_SYSCALL_CALL (execveat, dirfd, path, &argv[0], &envp[0], flags);
#ifndef __ASSUME_EXECVEAT
  if (errno != ENOSYS)
    return -1;

  if ((flags & ~(AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW)) != 0)
      return INLINE_SYSCALL_ERROR_RETURN_VALUE (EINVAL);

  return __execveat_fallback (dirfd, path, argv, envp, flags);
#endif

  return -1;
}
