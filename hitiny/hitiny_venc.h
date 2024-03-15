#ifndef __platform_x_mpi_venc_h__
#define __platform_x_mpi_venc_h__

#include "hi_comm_venc.h"

void hitiny_MPI_VENC_Init();
void hitiny_MPI_VENC_Done();

int hitiny_MPI_VENC_CreateGroup(VENC_GRP VeGroup);
int hitiny_MPI_VENC_DestroyGroup(VENC_GRP VeGroup);

int hitiny_MPI_VENC_CreateChn(VENC_CHN VeChn, const VENC_CHN_ATTR_S *pstAttr);
int hitiny_MPI_VENC_DestroyChn(VENC_CHN VeChn);

int hitiny_MPI_VENC_RegisterChn(VENC_GRP VeGroup, VENC_CHN VeChn);
int hitiny_MPI_VENC_UnRegisterChn(VENC_CHN VeChn);

int hitiny_MPI_VENC_GetFd(VENC_CHN VeChn);

int hitiny_MPI_VENC_StartRecvPic(VENC_CHN VeChn);
int hitiny_MPI_VENC_StopRecvPic(VENC_CHN VeChn);

int hitiny_MPI_VENC_Query(VENC_CHN VeChn, VENC_CHN_STAT_S *pstStat);
int hitiny_MPI_VENC_GetStream(VENC_CHN VeChn, VENC_STREAM_S *pstStream, HI_BOOL bBlockFlag);
int hitiny_MPI_VENC_ReleaseStream(VENC_CHN VeChn, VENC_STREAM_S *pstStream);

#endif // __platform_x_mpi_venc_h__

