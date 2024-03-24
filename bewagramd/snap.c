#include "snap.h"
#include "aux/logger.h"
#include <evcurl/evcurl.h>
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

extern evcurl_processor_t* g_evcurl_proc;
extern struct DaemonConfig g_dcfg;

static void PUT_snap_req_end_cb(evcurl_req_result_t* res, void* src_req_data)
{
    log_info("PUT snap Req DONE: %d\n", res->result);
    struct evcurl_upload_req_s* put_req = (struct evcurl_upload_req_s*)src_req_data;
    log_info("    upload data sz was %u\n", put_req->sz_buf);
    log_info("    effective URL: %s\n", res->effective_url);
    log_info("    %ld\n", res->response_code);
    log_info("    Content-Type: %s\n", res->content_type ? res->content_type : "(none)");

    if (!res->sz_body) return;

    log_info("\n\n%.*s\n--------------------------------------------------------------\n", (int)res->sz_body, (const char*)res->body);

    free(put_req->buf);
    free(put_req);
}

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

    struct evcurl_upload_req_s* upload_data = (struct evcurl_upload_req_s*)malloc(sizeof(struct evcurl_upload_req_s));
    upload_data->sz_buf = 0;
    for (unsigned iii = 0; iii < stStream.u32PackCount; iii++)
    {
        VENC_PACK_S* pstData = &(stStream.pstPack[iii]);
        upload_data->sz_buf += pstData->u32Len[0];
        upload_data->sz_buf += pstData->u32Len[1];
    }

    upload_data->buf = malloc(upload_data->sz_buf);
    upload_data->ptr = upload_data->buf;

    for (unsigned iii = 0; iii < stStream.u32PackCount; iii++)
    {
        VENC_PACK_S* pstData = &(stStream.pstPack[iii]);
        memcpy(upload_data->ptr, pstData->pu8Addr[0], pstData->u32Len[0]);
        upload_data->ptr += pstData->u32Len[0];
        memcpy(upload_data->ptr, pstData->pu8Addr[1], pstData->u32Len[1]);
        upload_data->ptr += pstData->u32Len[1];
    }

    upload_data->ptr = upload_data->buf;
    upload_data->url = g_dcfg.snap.http_PUT_snap_url;
    evcurl_new_UPLOAD(g_evcurl_proc, upload_data, PUT_snap_req_end_cb);

    log_info("%u from 0x%x to 0x%x", upload_data->sz_buf, upload_data->buf, upload_data->ptr);

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

int done_snap_machine(struct ev_loop* loop, const struct DaemonConfig* dc)
{
    ev_io_stop(loop, &venc_snap_ev_io);

    hitiny_MPI_VENC_StopRecvPic(SNAP_VENC_CHN_ID);
    hitiny_sys_unbind_VPSS_GROUP(0, (int)dc->snap.vpss_chn, SNAP_VENC_GRP_ID);
    hitiny_MPI_VENC_UnRegisterChn(SNAP_VENC_CHN_ID);
    hitiny_MPI_VENC_DestroyChn(SNAP_VENC_CHN_ID);
    hitiny_MPI_VENC_DestroyGroup(SNAP_VENC_GRP_ID);

    return 0;
}

