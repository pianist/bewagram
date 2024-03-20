#include "hitiny_sys.h"
#include <errno.h>
#include <string.h>
#include <aux/logger.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "hi_common.h"

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

typedef struct hitiny_sys_bind_param_s
{
    MPP_CHN_S src;
    MPP_CHN_S dest;
} hitiny_sys_bind_param_t;

int hitiny_sys_bind_VPSS_GROUP(int vpss_dev_id, int vpss_chn_id, int grp_dev_id, int grp_chn_id)
{
    if (fd_sys < 0)
    {
        fd_sys = hitiny_open_dev("/dev/sys");
        if (fd_sys < 0) return 0xA0028010;
    }

    hitiny_sys_bind_param_t param;
    param.src.enModId = HI_ID_VPSS;
    param.src.s32DevId = vpss_dev_id;
    param.src.s32ChnId = vpss_chn_id;

    param.dest.enModId = HI_ID_GROUP;
    param.dest.s32DevId = grp_dev_id;
    param.dest.s32ChnId = grp_chn_id;

    return ioctl(fd_sys, 0x40185907, &param);
}

int hitiny_sys_unbind_VPSS_GROUP(int vpss_dev_id, int vpss_chn_id, int grp_dev_id, int grp_chn_id)
{
    if (fd_sys < 0)
    {
        fd_sys = hitiny_open_dev("/dev/sys");
        if (fd_sys < 0) return 0xA0028010;
    }

    hitiny_sys_bind_param_t param;
    param.src.enModId = HI_ID_VPSS;
    param.src.s32DevId = vpss_dev_id;
    param.src.s32ChnId = vpss_chn_id;

    param.dest.enModId = HI_ID_GROUP;
    param.dest.s32DevId = grp_dev_id;
    param.dest.s32ChnId = grp_chn_id;

    return ioctl(fd_sys, 0x40185908, &param);
}

int hitiny_open_dev(const char* fname)
{
    struct stat stat_buf;

    if (stat(fname, &stat_buf) < 0)
    {
        log_error("ERROR: can't stat() on '%s', %d (%s)", fname, errno, strerror(errno));
        return -1;
    }

    if ((stat_buf.st_mode & 0xF000) != 0x2000)
    {
        log_error("ERROR: %s is not a device", fname);
        return -1;
    }

    int fd = open(fname, O_RDWR);
    if (fd < 0)
    {
        log_error("ERROR: can't open() on '%s', %d (%s)", fname, errno, strerror(errno));
        return -1;
    }

    return fd;
}

