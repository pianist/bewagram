#include "cfg.h"

#include <stdio.h>
#include <cfg/common.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#define SECTION_gpio        1
#define SECTION_button      2
#define SECTION_snap        3
#define SECTION_audio       4

static int __cfg_daemon_current_section = 0;
static char __cfg_daemon_error_key[32];
static char __cfg_daemon_error_value[256];

const char* cfg_daemon_read_error_value()
{
    return __cfg_daemon_error_value;
}

const char* cfg_daemon_read_error_key()
{
    return __cfg_daemon_error_key;
}

#define ADD_SECTION(section) if (strcmp(s, #section) == 0) { __cfg_daemon_current_section = SECTION_##section; return CFG_PROC_OK; }

static int __cfg_daemon_read_section_cb(const char* s)
{
    ADD_SECTION(gpio)
    ADD_SECTION(button)
    ADD_SECTION(snap)
    ADD_SECTION(audio)

    snprintf(__cfg_daemon_error_value, 256, "%s", s);
    return CFG_PROC_WRONG_SECTION;
}

struct DaemonConfig* __current_dc = 0;
#define KEYVAL_PARAM_COPY_STR(key, dest, MAX) if (strcasecmp(key, k) == 0) { snprintf(dest, MAX, "%s", v); return CFG_PROC_OK; }
#define KEYVAL_PARAM_IGN(key) if (strcasecmp(key, k) == 0) { return CFG_PROC_OK; }

#define KEYVAL_PARAM_SL_dec(key, dest) if (strcasecmp(key, k) == 0) {   \
        char* endptr;                                                   \
        dest = strtol(v, &endptr, 10);                                  \
        if (!*endptr) return CFG_PROC_OK;                               \
        snprintf(__cfg_daemon_error_key, 256, "%s", k);                 \
        snprintf(__cfg_daemon_error_value, 256, "%s", v);               \
        return CFG_PROC_VALUE_BAD;                                      \
    }

#define KEYVAL_PARAM_UL_dec(key, dest) if (strcasecmp(key, k) == 0) {   \
        char* endptr;                                                   \
        dest = strtoul(v, &endptr, 10);                                 \
        if (!*endptr) return CFG_PROC_OK;                               \
        snprintf(__cfg_daemon_error_key, 256, "%s", k);                 \
        snprintf(__cfg_daemon_error_value, 256, "%s", v);               \
        return CFG_PROC_VALUE_BAD;                                      \
    }

#define KEYVAL_PARAM_ENUM(key, dest, pvs) if (strcasecmp(key, k) == 0) {        \
        unsigned n = 0;                                                         \
        while (pvs[n]) {                                                        \
            if (strcmp(pvs[n], v) == 0) {                                       \
                dest = n; return CFG_PROC_OK;                                   \
            }                                                                   \
            n++;                                                                \
        }                                                                       \
        char* endptr;                                                           \
        unsigned x = strtoul(v, &endptr, 10);                                   \
        if (!*endptr && (n > x)) {                                              \
            dest = x; return CFG_PROC_OK;                                       \
        }                                                                       \
        snprintf(__cfg_daemon_error_key, 256, "%s", k);                         \
        snprintf(__cfg_daemon_error_value, 256, "%s", v);                       \
        return CFG_PROC_VALUE_BAD;                                              \
    }

static int __cfg_daemon_read_keyval_cb_audio(const char* k, const char* v)
{
    KEYVAL_PARAM_COPY_STR("majestic_compatible", __current_dc->audio.majestic_compatible, 3);
    KEYVAL_PARAM_UL_dec("bitrate", __current_dc->audio.bitrate);

    snprintf(__cfg_daemon_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

static int __cfg_daemon_read_keyval_cb_gpio(const char* k, const char* v)
{
    KEYVAL_PARAM_SL_dec("day_night_sensor", __current_dc->gpio.day_night_sensor)
    KEYVAL_PARAM_SL_dec("call_button", __current_dc->gpio.call_button)
    KEYVAL_PARAM_SL_dec("ircut_filter", __current_dc->gpio.ircut_filter)
    KEYVAL_PARAM_SL_dec("night_illumination", __current_dc->gpio.night_illumination)

    snprintf(__cfg_daemon_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

static int __cfg_daemon_read_keyval_cb_button(const char* k, const char* v)
{
    KEYVAL_PARAM_COPY_STR("http_GET_log", __current_dc->button.http_GET_log, 256);
    KEYVAL_PARAM_COPY_STR("music", __current_dc->button.music, 256);

    snprintf(__cfg_daemon_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

const char *cfg_daemon_vals_vpss_chn[] = { "VPSS_CHN_CHN0", "VPSS_CHN_CHN1", "VPSS_CHN_BYPASS", 0 };

static int __cfg_daemon_read_keyval_cb_snap(const char* k, const char* v)
{
    KEYVAL_PARAM_UL_dec("width", __current_dc->snap.width)
    KEYVAL_PARAM_UL_dec("height", __current_dc->snap.height)
    KEYVAL_PARAM_ENUM("vpss_chn", __current_dc->snap.vpss_chn, cfg_daemon_vals_vpss_chn);
    KEYVAL_PARAM_COPY_STR("http_PUT_snap_url", __current_dc->snap.http_PUT_snap_url, 256);

    snprintf(__cfg_daemon_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

#define KEYVAL_CASE(section) case SECTION_##section: { return __cfg_daemon_read_keyval_cb_##section(k, v); }

static int __cfg_daemon_read_keyval_cb(const char* k, const char* v)
{
    switch (__cfg_daemon_current_section)
    {
        KEYVAL_CASE(audio)
        KEYVAL_CASE(gpio)
        KEYVAL_CASE(button)
        KEYVAL_CASE(snap)
    }
    return CFG_PROC_OK;
}

int cfg_daemon_read(const char* fname, struct DaemonConfig* dc)
{
    __cfg_daemon_current_section = 0;
    __current_dc = dc;

    memset(dc, 0, sizeof(struct DaemonConfig));
    
    dc->snap.vpss_chn = VPSS_CHN_UNSET;

    return cfg_proc_read(fname, __cfg_daemon_read_section_cb, __cfg_daemon_read_keyval_cb);
}


