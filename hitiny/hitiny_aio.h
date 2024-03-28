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
#ifndef __HITINY_AIO_H__
#define __HITINY_AIO_H__

#include "hi_comm_aio.h"

int hitiny_init_acodec(AUDIO_SAMPLE_RATE_E enSample, HI_BOOL bMicIn);

int hitiny_MPI_AO_Init();
void hitiny_MPI_AO_Done();

int hitiny_MPI_AO_SetPubAttr(AUDIO_DEV AudioDevId, const AIO_ATTR_S *pstAttr);
int hitiny_MPI_AO_GetPubAttr(AUDIO_DEV AudioDevId, AIO_ATTR_S *pstAttr);

int hitiny_MPI_AO_Enable(AUDIO_DEV AudioDevId);
int hitiny_MPI_AO_Disable(AUDIO_DEV AudioDevId);

int hitiny_MPI_AO_EnableChn(AUDIO_DEV AudioDevId, AO_CHN AoChn);
int hitiny_MPI_AO_DisableChn(AUDIO_DEV AudioDevId, AO_CHN AoChn);

int hitiny_MPI_AO_GetFd(AUDIO_DEV AoDevId, AO_CHN AoChn);
int hitiny_MPI_AO_SendFrame(AUDIO_DEV AudioDevId, AO_CHN AoChn, const AUDIO_FRAME_S *pstData, HI_BOOL bBlock);

#endif // __HITINY_AIO_H__

