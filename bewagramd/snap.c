#include "snap.h"
#include "aux/logger.h"
#include <hitiny/hitiny_venc.h>
#include <hitiny/hitiny_sys.h>


#define CHECK_VALUE_BETWEEN(v, s, m, M) do                                                          \
                                        {                                                           \
                                            if (((v)<m) || ((v)>M))                                 \
                                            {                                                       \
                                                log_error("%s should be in [%u, %u]", s, m, M);     \
                                                return -1;                                          \
                                            }                                                       \
                                        } while(0);
                                            
#define TMP_VALUES_BUFFER_SZ 1024
#define SNAP_VENC_CHN_ID 63

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

int init_snap_machine(struct ev_loop* loop, const struct DaemonConfig* dc)
{
    if (init_snap_machine_checkcfg(dc)) return -1;

    log_info("SNAP machine init");

    int s32Ret = hitiny_MPI_VENC_CreateGroup(0);
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
        hitiny_MPI_VENC_DestroyGroup(0);
        return s32Ret;
    }

    s32Ret = hitiny_MPI_VENC_RegisterChn(0, SNAP_VENC_CHN_ID);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_RegisterChn failed with %#x!", s32Ret);
        hitiny_MPI_VENC_DestroyChn(SNAP_VENC_CHN_ID);
        hitiny_MPI_VENC_DestroyGroup(0);
        return s32Ret;
    }

    s32Ret = hitiny_sys_bind_VPSS_GROUP(0, (int)dc->snap.vpss_chn, SNAP_VENC_CHN_ID);
        if (HI_SUCCESS != s32Ret) {
        log_error("BIND VPSS to GRP failed with %#x!", s32Ret);
        hitiny_MPI_VENC_UnRegisterChn(SNAP_VENC_CHN_ID);
        hitiny_MPI_VENC_DestroyChn(SNAP_VENC_CHN_ID);
        hitiny_MPI_VENC_DestroyGroup(0);
        return s32Ret;
    }

    s32Ret = hitiny_MPI_VENC_StartRecvPic(SNAP_VENC_CHN_ID);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_StartRecvPic failed with %#x!\n", s32Ret);
        hitiny_sys_unbind_VPSS_GROUP(0, (int)dc->snap.vpss_chn, SNAP_VENC_CHN_ID);
        hitiny_MPI_VENC_UnRegisterChn(SNAP_VENC_CHN_ID);
        hitiny_MPI_VENC_DestroyChn(SNAP_VENC_CHN_ID);
        hitiny_MPI_VENC_DestroyGroup(0);
        return s32Ret;
    }

    return 0;
}

int done_snap_machine(const struct DaemonConfig* dc)
{
    hitiny_MPI_VENC_StopRecvPic(SNAP_VENC_CHN_ID);
    hitiny_sys_unbind_VPSS_GROUP(0, (int)dc->snap.vpss_chn, SNAP_VENC_CHN_ID);
    hitiny_MPI_VENC_UnRegisterChn(SNAP_VENC_CHN_ID);
    hitiny_MPI_VENC_DestroyChn(SNAP_VENC_CHN_ID);
    hitiny_MPI_VENC_DestroyGroup(0);

    return 0;
}

