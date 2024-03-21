#include "snap.h"
#include "aux/logger.h"
#include <hitiny/hitiny_venc.h>
#include <hitiny/hitiny_sys.h>
#include <stdlib.h>


#define CHECK_VALUE_BETWEEN(v, s, m, M) do                                                          \
                                        {                                                           \
                                            if (((v)<m) || ((v)>M))                                 \
                                            {                                                       \
                                                log_error("%s should be in [%u, %u]", s, m, M);     \
                                                return -1;                                          \
                                            }                                                       \
                                        } while(0);
                                            
#define TMP_VALUES_BUFFER_SZ 1024
#define SNAP_VENC_GRP_ID 5
#define SNAP_VENC_CHN_ID 11

static int init_snap_machine_checkcfg(const struct DaemonConfig* dc)
{
    CHECK_VALUE_BETWEEN(dc->snap.width, "snap width", 160, 1280)
    CHECK_VALUE_BETWEEN(dc->snap.height, "snap height", 90, 720)

    if (dc->snap.vpss_chn == VPSS_CHN_UNSET)
    {
        char buf[TMP_VALUES_BUFFER_SZ];
        unsigned buf_used = 0;

        const char** ptr = cfg_daemon_vals_vpss_chn;
        while (*ptr)
        {
            buf_used += snprintf(buf + buf_used, TMP_VALUES_BUFFER_SZ - buf_used, "%s%s", buf_used ? ", " : "", *ptr);
            ptr++;
        }
        
        log_crit("snap vpss_chn is not set, possible values: %s", buf);

        return -1;
    }
    return 0;
}

ev_io venc_snap_ev_io;

static void __venc_snap_cb(struct ev_loop *loop, ev_io* _w, int revents)
{
    log_info("event on fd = VENC: 0x%x ()", revents);
    hitiny_MPI_VENC_StopRecvPic(SNAP_VENC_CHN_ID);

    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;

    hitiny_MPI_VENC_Query(SNAP_VENC_CHN_ID, &stStat);
    stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
    stStream.u32PackCount = stStat.u32CurPacks;
    hitiny_MPI_VENC_GetStream(SNAP_VENC_CHN_ID, &stStream, HI_FALSE);
    hitiny_MPI_VENC_ReleaseStream(SNAP_VENC_CHN_ID, &stStream);
    free(stStream.pstPack);
    stStream.pstPack = NULL;

    log_info("DONE ONE JPEG");
}

int init_snap_machine(struct ev_loop* loop, const struct DaemonConfig* dc)
{
    if (init_snap_machine_checkcfg(dc)) return -1;

    log_info("SNAP machine init");

    int s32Ret = hitiny_MPI_VENC_CreateGroup(SNAP_VENC_GRP_ID);
    if (HI_SUCCESS != s32Ret)
    {
        log_error("MPI_VENC_CreateGroup failed with %#x!", s32Ret);
        return s32Ret;
    }

    VENC_CHN_ATTR_S stVencChnAttr;
    VENC_ATTR_JPEG_S stJpegAttr;

    stVencChnAttr.stVeAttr.enType = PT_JPEG;

    stJpegAttr.u32MaxPicWidth  = dc->snap.width;
    stJpegAttr.u32MaxPicHeight = dc->snap.height;
    stJpegAttr.u32PicWidth  = dc->snap.width;
    stJpegAttr.u32PicHeight = dc->snap.height;
    stJpegAttr.u32BufSize = dc->snap.width * dc->snap.height * 2;
    stJpegAttr.bByFrame = HI_TRUE;
    stJpegAttr.bVIField = HI_FALSE;
    stJpegAttr.u32Priority = 0;
    memcpy(&stVencChnAttr.stVeAttr.stAttrJpeg, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));

    s32Ret = hitiny_MPI_VENC_CreateChn(SNAP_VENC_CHN_ID, &stVencChnAttr);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_CreateChn failed with %#x!", s32Ret);
        hitiny_MPI_VENC_DestroyGroup(SNAP_VENC_GRP_ID);
        return s32Ret;
    }

    s32Ret = hitiny_MPI_VENC_RegisterChn(SNAP_VENC_GRP_ID, SNAP_VENC_CHN_ID);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_RegisterChn failed with %#x!", s32Ret);
        hitiny_MPI_VENC_DestroyChn(SNAP_VENC_CHN_ID);
        hitiny_MPI_VENC_DestroyGroup(SNAP_VENC_GRP_ID);
        return s32Ret;
    }

    s32Ret = hitiny_sys_bind_VPSS_GROUP(0, (int)dc->snap.vpss_chn, SNAP_VENC_GRP_ID);
        if (HI_SUCCESS != s32Ret) {
        log_error("BIND VPSS to GRP failed with %#x!", s32Ret);
        hitiny_MPI_VENC_UnRegisterChn(SNAP_VENC_CHN_ID);
        hitiny_MPI_VENC_DestroyChn(SNAP_VENC_CHN_ID);
        hitiny_MPI_VENC_DestroyGroup(SNAP_VENC_GRP_ID);
        return s32Ret;
    }

    int fd = hitiny_MPI_VENC_GetFd(SNAP_VENC_CHN_ID);
    if (fd < 0)
    {
        log_error("HI_MPI_VENC_GetFd failed with %#x!\n", s32Ret);
        hitiny_sys_unbind_VPSS_GROUP(0, (int)dc->snap.vpss_chn, SNAP_VENC_GRP_ID);
        hitiny_MPI_VENC_UnRegisterChn(SNAP_VENC_CHN_ID);
        hitiny_MPI_VENC_DestroyChn(SNAP_VENC_CHN_ID);
        hitiny_MPI_VENC_DestroyGroup(SNAP_VENC_GRP_ID);
        return fd;
    }

    ev_io_init(&venc_snap_ev_io, __venc_snap_cb, fd, EV_READ);
    ev_io_start(loop, &venc_snap_ev_io);

    hitiny_MPI_VENC_StartRecvPic(SNAP_VENC_CHN_ID);

    return 0;
}

void snap_machine_TAKE()
{
    hitiny_MPI_VENC_StartRecvPic(SNAP_VENC_CHN_ID);
}

int done_snap_machine(const struct DaemonConfig* dc)
{
    hitiny_MPI_VENC_StopRecvPic(SNAP_VENC_CHN_ID);
    hitiny_sys_unbind_VPSS_GROUP(0, (int)dc->snap.vpss_chn, SNAP_VENC_GRP_ID);
    hitiny_MPI_VENC_UnRegisterChn(SNAP_VENC_CHN_ID);
    hitiny_MPI_VENC_DestroyChn(SNAP_VENC_CHN_ID);
    hitiny_MPI_VENC_DestroyGroup(SNAP_VENC_GRP_ID);

    return 0;
}

