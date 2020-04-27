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

int
__execveat_fallback (int dirfd, const char *path, char *const argv[],
                   char *const envp[], int flags)
{
    int fd;

    if (path[0] == '\0' && (flags & AT_EMPTY_PATH) && dirfd >= 0)
        fd = dirfd;
    else
    {
        int oflags = O_CLOEXEC;
        if (flags & AT_SYMLINK_NOFOLLOW)
            oflags |= O_NOFOLLOW;
        fd = __openat_nocancel (dirfd, path, oflags);
    }
    if (fd < 0)
        return INLINE_SYSCALL_ERROR_RETURN_VALUE (EBADFD);

    struct fd_to_filename fdfilename;
    const char *gfilename = __fd_to_filename (fd, &fdfilename);

    /* We do not need the return value.  */
    __execve (gfilename, argv, envp);

    int save = errno;

    /* We come here only if the 'execve' call fails.  Determine whether
       /proc is mounted.  If not we return ENOSYS.  */
    struct stat64 st;
    if (__stat64 (FD_TO_FILENAME_PREFIX, &st) != 0 && errno == ENOENT)
        save = ENOSYS;

    if (fd != dirfd)
        __close_nocancel_nostatus (fd);
    __set_errno (save);

    return -1;
}
