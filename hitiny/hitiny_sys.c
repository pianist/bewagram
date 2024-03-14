#include "hitiny_sys.h"
#include <errno.h>
#include <string.h>
#include <aux/logger.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

static int fd_sys = -1;

int hitiny_MPI_SYS_Init()
{
    if (fd_sys >= 0) return 0;

    fd_sys = hitiny_open_dev("/dev/sys");
    if (fd_sys < 0)
    {
        return -1;
    }

    return ioctl(fd_sys, 0x5900);
}

void hitiny_MPY_SYS_Done()
{
    if (fd_sys != -1)
    {
        ioctl(fd_sys, 0x5901);
        close(fd_sys);
        fd_sys = -1;
    }
}

int hitiny_open_dev(const char* fname)
{
    struct stat stat_buf;

    if (stat(fname, &stat_buf) < 0)
    {
        log_error("ERROR: can't stat() on '%s', %d (%s)\n", fname, errno, strerror(errno));
        return -1;
    }

    if ((stat_buf.st_mode & 0xF000) != 0x2000)
    {
        log_error("ERROR: %s is not a device\n", fname);
        return -1;
    }

    int fd = open(fname, O_RDWR);
    if (fd < 0)
    {
        log_error("ERROR: can't open() on '%s', %d (%s)\n", fname, errno, strerror(errno));
        return -1;
    }

    return fd;
}

