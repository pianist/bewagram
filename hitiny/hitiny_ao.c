#include "hitiny_aio.h"
#include "hitiny_sys.h"
#include <poll.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <aux/logger.h>

#define MAX_AO_CHN_NUM 1

static int mpi_ao_dev_fd = -1;
static int mpi_ao_dev_chn_fds[MAX_AO_CHN_NUM + 1];

int hitiny_MPI_AO_Init()
{
    if (mpi_ao_dev_fd >= 0) return 0;

    mpi_ao_dev_fd = hitiny_open_dev("/dev/ao");
    if (mpi_ao_dev_fd < 0) return mpi_ao_dev_fd;

    unsigned param = 0;
    int ret = ioctl(mpi_ao_dev_fd, 0x40045800, &param);
    if (ret)
    {
        log_error("ERROR in hitiny_MPI_AO_Init/ioctl 0x40045800, error 0x%08x: %d (%s)\n", ret, errno, strerror(errno));
        return ret;
    }

    for (int i = 0; i <= MAX_AO_CHN_NUM; ++i)
    {
        mpi_ao_dev_chn_fds[i] = -1;
    }

    return 0;
}

void hitiny_MPI_AO_Done()
{
    for (int i = 0; i <= MAX_AO_CHN_NUM; ++i)
    {
        if (mpi_ao_dev_chn_fds[i] >= 0)
        {
            close(mpi_ao_dev_chn_fds[i]);
            mpi_ao_dev_chn_fds[i] = -1;
        }
    }

    if (mpi_ao_dev_fd >= 0)
    {
        close(mpi_ao_dev_fd);
        mpi_ao_dev_fd = -1;
    }
}

int hitiny_MPI_AO_SetPubAttr(AUDIO_DEV AoDevId, const AIO_ATTR_S *pstAioAttr)
{
    if (AoDevId > 0) return 0xA0168001;

    if (mpi_ao_dev_fd < 0) return 0xA0168010;

    return ioctl(mpi_ao_dev_fd, 0x40245801, pstAioAttr);
}

int hitiny_MPI_AO_Enable(AUDIO_DEV AoDevId)
{
    if (AoDevId > 0) return 0xA0168001;

    if (mpi_ao_dev_fd < 0) return 0xA0168010;

    return ioctl(mpi_ao_dev_fd, 0x5803);
}

int hitiny_MPI_AO_Disable(AUDIO_DEV AoDevId)
{
    if (AoDevId > 0) return 0xA0168001;

    if (mpi_ao_dev_fd < 0) return 0xA0168010;

    return ioctl(mpi_ao_dev_fd, 0x5804);
}

int hitiny_MPI_AO_EnableChn(AUDIO_DEV AoDevId, AO_CHN AoChn)
{
    int fd = hitiny_MPI_AO_GetFd(AoDevId, AoChn);
    if (fd <= 0) return fd;

    return ioctl(fd, 0x5808);
}

int hitiny_MPI_AO_DisableChn(AUDIO_DEV AoDevId, AO_CHN AoChn)
{
    int fd = hitiny_MPI_AO_GetFd(AoDevId, AoChn);
    if (fd <= 0) return fd;

    return ioctl(fd, 0x5809);
}

int hitiny_MPI_AO_GetFd(AUDIO_DEV AoDevId, AO_CHN AoChn)
{
    if (AoDevId > 0) return 0xA0168001;

    if (AoChn > MAX_AO_CHN_NUM) return 0xA0168002;

    int fd = mpi_ao_dev_chn_fds[AoChn];
    if (fd > 0) return fd;

    fd = hitiny_open_dev("/dev/ao");
    if (fd < 0) return fd;

    unsigned param = AoChn;
    int ret = ioctl(fd, 0x40045800, &param);
    if (ret)
    {
        log_error("ERROR in hitiny_MPI_AO_GetFd/ioctl 0x40045800, error 0x%08x: %d (%s)\n", ret, errno, strerror(errno));
        close(fd);
        return ret;
    }

    mpi_ao_dev_chn_fds[AoChn] = fd;

    return fd;
}

int hitiny_MPI_AO_SendFrame(AUDIO_DEV AudioDevId, AO_CHN AoChn, const AUDIO_FRAME_S* frame, HI_BOOL blocking_mode)
{
    int fd = hitiny_MPI_AO_GetFd(AudioDevId, AoChn);

    if (fd <= 0) return fd;

    if (!blocking_mode)
    {
        struct pollfd x;
        x.fd = fd; 
        x.events = POLLOUT;
        poll(&x, 1, 0); 
        if (x.events != POLLOUT) return 0xA016800F;
    }

    int result = ioctl(fd, 0x40305805, frame);

    return result;
}

