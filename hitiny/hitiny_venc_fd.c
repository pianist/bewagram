#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <aux/logger.h>
#include <sys/mman.h>

#include "hitiny_venc.h"
#include "internal.h"

static int g_hitiny_venc_init_ok = 0;

static int g_hitiny_fd_mem = -1;
static int g_hitiny_fd_grp[VENC_MAX_GRP_NUM];
static hitiny_MPI_VENC_CHN g_hitiny_venc_chn[VENC_MAX_CHN_NUM];

void* hitinity_VencVirt2Phy(unsigned vc, void* a)
{
    return (g_hitiny_venc_chn[vc].base_phy - g_hitiny_venc_chn[vc].base_virtual + a);
}

void* hitinity_VencVirt2User(unsigned vc, void* a)
{
    return (g_hitiny_venc_chn[vc].base_user - g_hitiny_venc_chn[vc].base_virtual + a);
}

void* hitinity_VencUser2Virt(unsigned vc, void* a)
{
    return (g_hitiny_venc_chn[vc].base_virtual - g_hitiny_venc_chn[vc].base_user + a);
}

void hitiny_MPI_VENC_Init()
{
    if (g_hitiny_venc_init_ok) return;

    g_hitiny_fd_mem = -1;

    memset(&g_hitiny_venc_chn, 0, sizeof(hitiny_MPI_VENC_CHN) * VENC_MAX_CHN_NUM);

    for (int i = 0; i < VENC_MAX_CHN_NUM; ++i)
    {
        g_hitiny_venc_chn[i].chn_fd = -1;
    }

    for (int i = 0; i < VENC_MAX_GRP_NUM; ++i)
    {
        g_hitiny_fd_grp[i] = -1;
    }

    g_hitiny_venc_init_ok = 1;
}

void hitiny_MPI_VENC_Done()
{
    if (!g_hitiny_venc_init_ok) return;

    for (int i = 0; i < VENC_MAX_CHN_NUM; ++i)
    {
        if (g_hitiny_venc_chn[i].chn_fd >= 0) close(g_hitiny_venc_chn[i].chn_fd);
        g_hitiny_venc_chn[i].chn_fd = -1;
    }

    for (int i = 0; i < VENC_MAX_GRP_NUM; ++i)
    {
        if (g_hitiny_fd_grp[i] >= 0) close(g_hitiny_fd_grp[i]);
        g_hitiny_fd_grp[i] = -1;
    }

    if (g_hitiny_fd_mem >= 0)
    {
        close(g_hitiny_fd_mem);
        g_hitiny_fd_mem = -1;
    }

    g_hitiny_venc_init_ok = 0;
}

static int open_grp_fd(VENC_GRP VeGrp)
{
    char fname[32];
    struct stat stat_buf;
    snprintf(fname, 32, "/dev/grp");
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

    int _new_grp_fd = open(fname, 2, 0);
    if (_new_grp_fd < 0)
    {
        log_error("ERROR: can't open() on '%s', %d (%s)", fname, errno, strerror(errno));
        return -1;
    }

    unsigned req_data = VeGrp;
    int ret = ioctl(_new_grp_fd, 0x40044703, &req_data);
    if (ret)
    {
        close(_new_grp_fd);
        log_error("ERROR: can't ioctl(0x40044518) for group %u: err 0x%x", VeGrp, ret);

        return -1;
    }

    return _new_grp_fd;
}

int hitiny_MPI_VENC_CreateGroup(VENC_GRP VeGrp)
{
    if (VeGrp >= VENC_MAX_GRP_NUM) return 0xA0078001;

    if (!g_hitiny_venc_init_ok) return 0xA0078010;

    int fd = g_hitiny_fd_grp[VeGrp];

    if (fd < 0)
    {
        fd = open_grp_fd(VeGrp);
        g_hitiny_fd_grp[VeGrp] = fd;
    }

    if (fd < 0)
    {
        log_error("Open group error in hitiny_MPI_VENC_CreateGroup, system may be not ready");
        return 0xA0078010;
    }

    return ioctl(fd, 0x4700u);
};

int hitiny_MPI_VENC_DestroyGroup(VENC_GRP VeGrp)
{
    if (VeGrp >= VENC_MAX_GRP_NUM) return 0xA0078001;

    if (!g_hitiny_venc_init_ok) return 0xA0078010;

    int fd = g_hitiny_fd_grp[VeGrp];

    if (fd < 0)
    {
        fd = open_grp_fd(VeGrp);
        g_hitiny_fd_grp[VeGrp] = fd;
    }

    if (fd < 0)
    {
        log_error("Open group error in hitiny_MPI_VENC_DestroyGroup, system may be not ready");
        return 0xA0078010;
    }

    int ret = ioctl(fd, 0x4701u);

    close(fd);
    g_hitiny_fd_grp[VeGrp] = -1;

    return ret;
}

static int open_chn_fd(VENC_CHN VeChn)
{
    char fname[32];
    struct stat stat_buf;
    snprintf(fname, 32, "/dev/venc");
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

    int _new_chn_fd = open(fname, 2, 0);
    if (_new_chn_fd < 0)
    {
        log_error("ERROR: can't open() on '%s', %d (%s)", fname, errno, strerror(errno));
        return -1;
    }

    unsigned req_data = VeChn;
    if (ioctl(_new_chn_fd, 0x40044518u, &req_data))
    {
        close(_new_chn_fd);
        log_error("ERROR: can't ioctl(0x40044518u) for channel %u", VeChn);

        return -1;
    }

    return _new_chn_fd;
}

int hitiny_MPI_VENC_GetFd(VENC_CHN VeChn)
{
    if (VeChn >= VENC_MAX_CHN_NUM) return 0xA0078002;

    if (!g_hitiny_venc_init_ok) return 0xA0078010;

    int fd = g_hitiny_venc_chn[VeChn].chn_fd;

    if (fd < 0)
    {
        fd = open_chn_fd(VeChn);
        g_hitiny_venc_chn[VeChn].chn_fd = fd;

        if (-1 == fd)
        {
            return 0xA0078010;
        }
    }

    return fd;
}

int hitiny_MPI_VENC_RegisterChn(VENC_GRP VeGroup, VENC_CHN VeChn)
{
    int fd = hitiny_MPI_VENC_GetFd(VeChn);

    if (fd < 0) return fd;

    unsigned param = VeGroup;

    return ioctl(fd, 0x40044508u, &param);
}

int hitiny_MPI_VENC_UnRegisterChn(VENC_CHN VeChn)
{
    int fd = hitiny_MPI_VENC_GetFd(VeChn);

    if (fd < 0) return fd;

    unsigned param = VeChn;

    return ioctl(fd, 0x4509u, &param);
}

int hitiny_MPI_VENC_StartRecvPic(VENC_CHN VeChn)
{
    int fd = hitiny_MPI_VENC_GetFd(VeChn);

    if (fd < 0) return fd;

    return ioctl(fd, 0x450Au);
}

int hitiny_MPI_VENC_StopRecvPic(VENC_CHN VeChn)
{
    int fd = hitiny_MPI_VENC_GetFd(VeChn);

    if (fd < 0) return fd;

    return ioctl(fd, 0x450Bu);
}

int hitiny_MPI_VENC_Query(VENC_CHN VeChn, VENC_CHN_STAT_S *pstStat)
{
    if (!pstStat)
    {
        log_error("ERROR: pstStat is NULL");
        return 0xA0078006;
    }

    int fd = hitiny_MPI_VENC_GetFd(VeChn);

    if (fd < 0) return fd;

    return ioctl(fd, 0x801C450E, pstStat);
}

static int open_all_grp_fd()
{
    char fname[32];
    struct stat stat_buf;
    snprintf(fname, 32, "/dev/grp");
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

    int _new_grp_fd = open(fname, 2, 0);
    if (_new_grp_fd < 0)
    {
        log_error("ERROR: can't open() on '%s', %d (%s)", fname, errno, strerror(errno));
        return -1;
    }

    return _new_grp_fd;
}

int hitiny_MPI_VENC_SetColor2GreyConf(const GROUP_COLOR2GREY_CONF_S *pstGrpColor2GreyConf)
{
    if (!pstGrpColor2GreyConf)
    {
        log_error("ERROR: pstGrpColor2GreyConf is NULL");
        return 0xA0078006;
    }

    int fd = open_all_grp_fd();
    if (fd < 0) return fd;

    int ret = ioctl(fd, 0x400C4708, pstGrpColor2GreyConf);

    close(fd);
    return ret;
}

int hitiny_MPI_VENC_SetGrpColor2Grey(VENC_GRP VeGrp, const GROUP_COLOR2GREY_S *pstGrpColor2Grey)
{
    if (VeGrp >= VENC_MAX_GRP_NUM) return 0xA0078001;

    if (!pstGrpColor2Grey)
    {
        log_error("ERROR: pstGrpColor2GreyConf is NULL");
        return 0xA0078006;
    }

    int fd = g_hitiny_fd_grp[VeGrp];

    if (fd < 0)
    {
        fd = open_grp_fd(VeGrp);
        g_hitiny_fd_grp[VeGrp] = fd;
    }

    if (fd < 0) return fd;

    return ioctl(fd, 0x40044706, pstGrpColor2Grey);
}

int hitiny_MPI_VENC_CreateChn(VENC_CHN VeChn, const VENC_CHN_ATTR_S *pstAttr)
{
    if (!pstAttr)
    {
        log_error("hitiny_MPI_VENC_CreateChn: pstAttr is NULL pointer");
        return 0xA0078006;
    }

    int fd = hitiny_MPI_VENC_GetFd(VeChn);

    if (fd < 0) return fd;

    if (g_hitiny_fd_mem < 0)
    {
        g_hitiny_fd_mem = open("/dev/venc", 4162);
        if (g_hitiny_fd_mem < 0)
        {
            log_error("hitiny_MPI_VENC_CreateChn: can't open(/dev/venc) %d (%s)", errno, strerror(errno));
            return 0xA0078010;
        }
    }

    VENC_CHN_ATTR_S param_attr;
    param_attr = *pstAttr;

    unsigned param_ret[3];
    int ret = ioctl(fd, 0x40544500u, &param_attr);
    if (!ret)
    {
        ret = ioctl(fd, 0x800C450Fu, param_ret);
    }

    if (ret)
    {
        log_error("hitiny_MPI_VENC_CreateChn: ioctl error 0x%x", ret);

        ioctl(fd, 0x4501);

        close(fd);
        g_hitiny_venc_chn[VeChn].chn_fd = -1;

        return ret;
    }

#ifdef __STREAM_BUF_DBG__
    log_info("YOYOYO: 0x%08x 0x%08x 0x%08x;", param_ret[0], param_ret[1], param_ret[2]);
#endif //__STREAM_BUF_DBG__

    g_hitiny_venc_chn[VeChn].base_phy = param_ret[0];
    g_hitiny_venc_chn[VeChn].base_virtual = param_ret[1];
    unsigned offset_base = param_ret[0] & 0xFFFFF000;
    unsigned offset_base_diff = param_ret[0] - offset_base;
    unsigned sz = 4096 + ((param_ret[2] - 1 + offset_base_diff) & 0xFFFFF000);
    g_hitiny_venc_chn[VeChn].base_user = 0;
    g_hitiny_venc_chn[VeChn].base_sz = param_ret[2];
    g_hitiny_venc_chn[VeChn].buf_mmap_sz = sz;


    void* ptr = mmap(0, 2 * sz, PROT_READ | PROT_WRITE, MAP_SHARED, g_hitiny_fd_mem, offset_base);
    g_hitiny_venc_chn[VeChn].buf_mmap = ptr;

    if (MAP_FAILED != ptr)
    {
        g_hitiny_venc_chn[VeChn].base_user = (unsigned)(ptr) + offset_base_diff;
        return 0;
    }
    else
    {
        log_error("hitiny_MPI_VENC_CreateChn: ioctl error 0x%x", ret);
    }

    ioctl(fd, 0x4501);

    close(fd);
    g_hitiny_venc_chn[VeChn].chn_fd = -1;

    return 0xA007800C;
}

int hitiny_MPI_VENC_DestroyChn(VENC_CHN VeChn)
{
    int fd = hitiny_MPI_VENC_GetFd(VeChn);

    if (fd < 0) return fd;

    int ret = ioctl(fd, 0x4501);

    if (!ret && g_hitiny_venc_chn[VeChn].base_user)
    {
        int ret = munmap((void*)g_hitiny_venc_chn[VeChn].base_user, 2 * g_hitiny_venc_chn[VeChn].buf_mmap_sz);
        g_hitiny_venc_chn[VeChn].base_user = 0;
        g_hitiny_venc_chn[VeChn].base_phy = 0;
        g_hitiny_venc_chn[VeChn].base_virtual = 0;
    }

    return ret;
}

int hitiny_MPI_VENC_GetStream(VENC_CHN VeChn, VENC_STREAM_S *pstStream, HI_BOOL bBlockFlag)
{
    int fd = hitiny_MPI_VENC_GetFd(VeChn);

    if (fd < 0) return fd;

    if (!pstStream || !pstStream->pstPack)
    {
        log_error("hitiny_MPI_VENC_GetStream: pstStream is NULL pointer");
        return 0xA0078006;
    }

    HI_BOOL bBlockMode = bBlockFlag;
    int ret;

    ret = ioctl(fd, 0x40044512u, &bBlockMode);
    if (ret)
    {
        log_error("hitiny_MPI_VENC_GetStream: ioctl 0x40044512 failed");
        return ret;
    }

    ret = ioctl(fd, 0xC040450Cu, pstStream);
    if (ret)
    {
        log_error("hitiny_MPI_VENC_GetStream: ioctl 0xC040450Cu failed");
        return ret;
    }

    for (unsigned i = 0; i < pstStream->u32PackCount; ++i)
    {
        VENC_PACK_S *p = &pstStream->pstPack[i];

#ifdef __STREAM_BUF_DBG__
        log_info("PACK[%u]:", i);
        log_info("\tDataType: %u", p->DataType);
        log_info("\tu32Len: [%u, %u]", p->u32Len[0], p->u32Len[1]);
        log_info("\tu32PhyAddr: [0x%x, 0x%x]", p->u32PhyAddr[0], p->u32PhyAddr[1]);
        log_info("\tu32pu8Addr: [0x%x, 0x%x]", p->pu8Addr[0], p->pu8Addr[1]);
#endif

        p->u32Len[0] -= 64;
        void* phyaddr0 = hitinity_VencVirt2Phy(VeChn, p->pu8Addr[0]);
        p->u32PhyAddr[0] = (unsigned)(phyaddr0) + 64;
        void* phyaddr1 = hitinity_VencVirt2Phy(VeChn, p->pu8Addr[1]);
        p->u32PhyAddr[1] = (unsigned)phyaddr1;

        void* addr0 = hitinity_VencVirt2User(VeChn, p->pu8Addr[0]);
        p->pu8Addr[0] = (HI_U8 *)(addr0 + 64);
        void* addr1 = hitinity_VencVirt2User(VeChn, p->pu8Addr[1]);
        p->pu8Addr[1] = (HI_U8 *)(addr1);

#ifdef __STREAM_BUF_DBG__
        log_info("\t\tu32Len: [%u, %u]", p->u32Len[0], p->u32Len[1]);
        log_info("\t\tu32PhyAddr: [0x%x, 0x%x]", p->u32PhyAddr[0], p->u32PhyAddr[1]);
        log_info("\t\tu32pu8Addr: [0x%x, 0x%x]", p->pu8Addr[0], p->pu8Addr[1]);
#endif
    }

    return 0;
}

int hitiny_MPI_VENC_ReleaseStream(VENC_CHN VeChn, VENC_STREAM_S *pstStream)
{
    int fd = hitiny_MPI_VENC_GetFd(VeChn);

    if (fd < 0) return fd;

    if (!pstStream || !pstStream->pstPack)
    {
        log_error("xHI_MPI_VENC_ReleaseStream: pstStream is NULL pointer");
        return 0xA0078006;
    }
    for (unsigned i = 0; i < pstStream->u32PackCount; ++i)
    {
        VENC_PACK_S *p = &pstStream->pstPack[i];

#ifdef __STREAM_BUF_DBG__
        log_info("PACK[%u]:", i);
        log_info("\tDataType: %u", p->DataType);
        log_info("\tu32Len: [%u, %u]", p->u32Len[0], p->u32Len[1]);
        log_info("\tu32PhyAddr: [0x%x, 0x%x]", p->u32PhyAddr[0], p->u32PhyAddr[1]);
        log_info("\tu32pu8Addr: [0x%x, 0x%x]", p->pu8Addr[0], p->pu8Addr[1]);
#endif

        p->u32Len[0] += 64;

        void* addr0 = hitinity_VencUser2Virt(VeChn, p->pu8Addr[0]);
        p->pu8Addr[0] = (HI_U8 *)(addr0 - 64);
        void* addr1 = hitinity_VencUser2Virt(VeChn, p->pu8Addr[1]);
        p->pu8Addr[1] = (HI_U8 *)(addr1);

#ifdef __STREAM_BUF_DBG__
        log_info("\t\tu32Len: [%u, %u]", p->u32Len[0], p->u32Len[1]);
        log_info("\t\tu32PhyAddr: [0x%x, 0x%x]", p->u32PhyAddr[0], p->u32PhyAddr[1]);
        log_info("\t\tu32pu8Addr: [0x%x, 0x%x]", p->pu8Addr[0], p->pu8Addr[1]);
#endif
    }

    return ioctl(fd, 0x4040450Du, pstStream);
}



