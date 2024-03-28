/*
 * Copyright (C) 2024 Alexander Pankov (pianisteg.mobile@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 */
#include "hitiny_sys.h"
#include "hitiny_aio.h"
#include <errno.h>
#include <string.h>
#include <aux/logger.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "acodec.h"

int hitiny_init_acodec(AUDIO_SAMPLE_RATE_E enSample, HI_BOOL bMicIn)
{
    int fd_acodec = -1;

    fd_acodec = hitiny_open_dev("/dev/acodec");
    if (fd_acodec < 0) return fd_acodec;

    int ret = 0;
    ret = ioctl(fd_acodec, ACODEC_SOFT_RESET_CTRL);
    if (0 != ret)
    {
        log_error("ERROR in hitiny_init_acodec/ACODEC_SOFT_RESET_CTRL: %d (%s)\n", errno, strerror(errno));
        close(fd_acodec);
        return ret;
    }

    unsigned int i2s_fs_sel = 0;
    switch (enSample)
    {
        case AUDIO_SAMPLE_RATE_8000:
        case AUDIO_SAMPLE_RATE_11025:
        case AUDIO_SAMPLE_RATE_12000:
            i2s_fs_sel = 0x18;
            break;

        case AUDIO_SAMPLE_RATE_16000:
        case AUDIO_SAMPLE_RATE_22050:
        case AUDIO_SAMPLE_RATE_24000:
            i2s_fs_sel = 0x19;
            break;
        case AUDIO_SAMPLE_RATE_32000:
        case AUDIO_SAMPLE_RATE_44100:
        case AUDIO_SAMPLE_RATE_48000:
            i2s_fs_sel = 0x1a;
            break;

        default:
            log_error("ERROR in hitiny_init_acodec/AUDIO_SAMPLE_RATE: %d (%s)\n", errno, strerror(errno));
            close(fd_acodec);
            return -1;
    }

    if (ioctl(fd_acodec, ACODEC_SET_I2S1_FS, &i2s_fs_sel))
    {
        log_error("ERROR in hitiny_init_acodec/ACODEC_SET_I2S1_FS: %d (%s)\n", errno, strerror(errno));
        ret = -1;
    }

    close(fd_acodec);
    return ret;
}

