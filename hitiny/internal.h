#ifndef __hitiny_mpi_venc_internal_h__
#define __hitiny_mpi_venc_internal_h__

typedef struct hitiny_MPI_VENC_CHN_S
{
    int chn_fd;

    unsigned base_phy;
    unsigned base_virtual;
    unsigned base_user;
    unsigned base_sz;
    void* buf_mmap;
    unsigned buf_mmap_sz;
} hitiny_MPI_VENC_CHN;

#endif // __hitiny_mpi_venc_internal_h__

