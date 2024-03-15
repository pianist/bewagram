#include "call_button.h"
#include <aux/logger.h>
#include <evcurl/evcurl.h>
#include <pthread.h>
#include <hitiny/hitiny_sys.h>
#include <hitiny/hitiny_aio.h>
#include "cfg.h"

extern struct DaemonConfig g_dcfg;
extern int stop_flag;

/* from here goes audio */

#include <unistd.h>
#include <stdlib.h>

#define AUDIO_PTNUMPERFRM   320

int playmusic_audio_init()
{
    AUDIO_DEV AoDev = 0;
    AO_CHN AoChn = 0;

    AIO_ATTR_S stAioAttr;
    memset(&stAioAttr, 0, sizeof(AIO_ATTR_S));

    stAioAttr.enSamplerate = 48000;
    stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag = 1;
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32PtNumPerFrm = AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt = 2;
    stAioAttr.u32ClkSel = 1;

    int ret = hitiny_init_acodec(48000, HI_TRUE);
    if (ret < 0)
    {
        log_error("hitiny_init_acodec() failed: 0x%X", ret);
        return -1;
    }

    ret = hitiny_MPI_AO_Init();
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_Init() failed: 0x%X", ret);
        return ret;
    }

    ret = hitiny_MPI_AO_Disable(0);
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_DisableDev: 0x%X", ret);
        return ret;
    }
    ret = hitiny_MPI_AO_SetPubAttr(0, &stAioAttr);
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_SetPubAttr failed: 0x%X", ret);
        return ret;
    }

    ret = hitiny_MPI_AO_Enable(0);
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_EnableDev: 0x%X", ret);
        return ret;
    }

    ret = hitiny_MPI_AO_EnableChn(0, 0);
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_EnableChn: 0x%X", ret);
        return ret;
    }

    return 0;
}

int playmusic_audio_done()
{
    int ret = hitiny_MPI_AO_DisableChn(0, 0);
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_DisableChn: 0x%X", ret);
        return ret;
    }

    ret = hitiny_MPI_AO_Disable(0);
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_DisableDev: 0x%X", ret);
        return ret;
    }

    hitiny_MPI_AO_Done();

    return 0;
}

static pthread_mutex_t playmusic_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t playmusic_th = 0;
static int playmusic_busy = 0;
static int playmusic_starting = 0;
static int playmusic_stop_flag = 0;

int playmusic_try_lock()
{
    pthread_mutex_lock(&playmusic_mutex);

    if (playmusic_busy || playmusic_starting)
    {
        log_warn("Music is busy, don't start new");
        pthread_mutex_unlock(&playmusic_mutex);
        return -1;
    }

    playmusic_starting = 1;
    pthread_mutex_unlock(&playmusic_mutex);

    if (playmusic_th)
    {
        log_info("join prev playmusic thread");
        pthread_join(playmusic_th, 0);
    }

    pthread_mutex_lock(&playmusic_mutex);
    playmusic_th = 0;
    playmusic_starting = 0;
    playmusic_busy = 1;
    pthread_mutex_unlock(&playmusic_mutex);

    return 0;
}

int playmusic_play_file(FILE* f, int* stop_flag)
{
    HI_U32 u32Len = 640;
    HI_U32 u32ReadLen;
    HI_U8 *pu8AudioStream = NULL;

    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8)*MAX_AUDIO_STREAM_LEN);
    if (!pu8AudioStream) return -1;

    unsigned id = 0;
    int res = 0;

    while (!res && !playmusic_stop_flag && (!stop_flag || !*stop_flag))
    {
        u32ReadLen = fread(pu8AudioStream, 1, u32Len, f);
        if (u32ReadLen <= 0)
        {
            free(pu8AudioStream);
            return 0;
        }
        AUDIO_FRAME_S frame;
        frame.enBitwidth = AUDIO_BIT_WIDTH_16;
        frame.enSoundmode = AUDIO_SOUND_MODE_MONO;
        frame.pVirAddr[0] = pu8AudioStream;
        frame.pVirAddr[1] = 0;
        frame.u32PhyAddr[0] = 0;
        frame.u32PhyAddr[1] = 0;
        frame.u64TimeStamp = 0;
        frame.u32Seq = id;
        frame.u32Len = u32ReadLen;
        frame.u32PoolId[0] = 0;
        frame.u32PoolId[1] = 0;

        res = hitiny_MPI_AO_SendFrame(0, 0, &frame, HI_TRUE);

        id++;
    }
    free(pu8AudioStream);
    return 1;
}

void* playmusic_play_th_proc(void* p)
{
    log_info("play %s", g_dcfg.button.music);

    int res = playmusic_audio_init();
    if (!res)
    {
        FILE* f = fopen(g_dcfg.button.music, "r");
        if (f)
        {
            log_info("Now play %s!", g_dcfg.button.music);
            playmusic_play_file(f, &stop_flag);
            fclose(f);
        }
        else
        {
            log_error("ERROR %d", errno);
            log_error("Error %d: %s", errno, strerror(errno));
        }
    }
    else
    {
        log_error("playmusic_audio_init error: %d", res);
    }

    playmusic_audio_done();

    pthread_mutex_lock(&playmusic_mutex);
    playmusic_busy = 0;
    pthread_mutex_unlock(&playmusic_mutex);

    return 0;
}

/* here ends audio */

extern evcurl_processor_t* g_evcurl_proc;
extern struct DaemonConfig g_dcfg;

static void test_req_cb(evcurl_req_result_t* res)
{
    log_info("Req tst DONE: %d\n", res->result);
    log_info("    effective URL: %s\n", res->effective_url);
    log_info("    %ld\n", res->response_code);
    log_info("    Content-Type: %s\n", res->content_type ? res->content_type : "(none)");
}

static void call_button_cb(int gpio_num, char state)
{
    log_info("KNOPKA %s", state == '1' ? "otpustili" : "nazhali");

    if ('0' == state)
    {
        playmusic_stop_flag = 1;
        if (strlen(g_dcfg.button.http_GET_log) > 0)
        {
            log_info("sending log request: %s", g_dcfg.button.http_GET_log);
            evcurl_new_http_GET(g_evcurl_proc, g_dcfg.button.http_GET_log, test_req_cb);
        }
    }
    else
    {
        log_warn("NOW PLAY MUSIC");
        if (!playmusic_try_lock())
        {
            playmusic_stop_flag = 0;
            pthread_create(&playmusic_th, 0, playmusic_play_th_proc, 0);
        }
    }
}

int init_button_evloop(struct ev_loop* loop, const struct DaemonConfig* dc)
{
    if (dc->gpio.call_button <= 0)
    {
        log_error("Incorrect call_button GPIO (%d)", dc->gpio.call_button);
        return -1;
    }

    int ret = evgpio_watcher_init(loop, dc->gpio.call_button, call_button_cb);
    if (ret < 0)
    {
        log_error("Can't open gpio %d: %s\n", dc->gpio.call_button, strerror(errno));
        return -1;
    }

    log_warn("init button gpio done");
    return 0;
}

