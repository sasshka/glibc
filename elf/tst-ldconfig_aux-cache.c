#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <ftw.h>
#include <stdint.h>

#include <support/check.h>
#include <support/support.h>

static int
display_info(const char *fpath, const struct stat *sb,
             int tflag, struct FTW *ftwbuf)
{
    printf("%-3s %2d %7jd   %-40s %d %s\n",
        (tflag == FTW_D) ?   "d"   : (tflag == FTW_DNR) ? "dnr" :
        (tflag == FTW_DP) ?  "dp"  : (tflag == FTW_F) ?   "f" :
        (tflag == FTW_NS) ?  "ns"  : (tflag == FTW_SL) ?  "sl" :
        (tflag == FTW_SLN) ? "sln" : "???",
        ftwbuf->level, (intmax_t) sb->st_size,
        fpath, ftwbuf->base, fpath + ftwbuf->base);
    return 0;           /* To tell nftw() to continue */
}

static int
do_test(void)
{
    char *args[] = { (char *) "/sbin/ldconfig", NULL };
    struct stat fs;
    long  int size, new_size, i;
    int status;
    pid_t pid = fork();

    if (!pid)
        execv(args[0], args);
    else {
        if (pid) {
            waitpid(pid, &status, 0);
            if (!(WIFEXITED(status)))
                 FAIL_EXIT1("ldconfig was aborted");
            if (stat("/var/cache/ldconfig/aux-cache", &fs) < 0) {
                if (errno == ENOENT)
                    FAIL_EXIT1("aux-cache does not exist\n");
                else
                    FAIL_EXIT1("Failed to open aux-cache.");
                return -1;
            }

            size = fs.st_size;
            for (i = 3; i > 0; i--) {
                new_size = size * i/4;
                truncate("/var/cache/ldconfig/aux-cache", new_size);
                if (errno)
                    FAIL_EXIT1("Truncation failed.");
                if (nftw("/var/cache/ldconfig/aux-cache", display_info, 1000, 0)
                    == -1) {
                    FAIL_EXIT1("nftw failed.");
                    return -1;
                }

                pid = fork();
                if (!pid)
                    execv(args[0], args);
                else {
                    waitpid(pid, &status, 0);
                    if (!(WIFEXITED(status)))
                        FAIL_EXIT1("ldconfig was aborted");
                }

            }
        }
    }

    return 0;
}

#include <support/test-driver.c>
