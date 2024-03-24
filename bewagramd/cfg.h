#pragma once

#include <cfg/common.h>

struct Device_GPIOs {
    int day_night_sensor;
    int call_button;
    int ircut_filter;
    int night_illumination;
};

struct ButtonConfig {
    char http_GET_log[256];
    char music[256];
};

enum VPSS_CHN_E
{
    VPSS_CHN_UNSET  = -1,
    VPSS_CHN_CHN0   = 0,
    VPSS_CHN_CHN1   = 1,
    VPSS_CHN_BYPASS = 2,
};

extern const char *cfg_daemon_vals_vpss_chn[];

struct SnapConfig {
    unsigned width;
    unsigned height;
    enum VPSS_CHN_E vpss_chn;
    char http_PUT_snap_url[256];
};

struct DaemonConfig {
    struct Device_GPIOs gpio;
    struct ButtonConfig button;
    struct SnapConfig snap;
};

int cfg_daemon_read(const char* fname, struct DaemonConfig* sc);
const char* cfg_daemon_read_error_key();
const char* cfg_daemon_read_error_value();

void cfg_daemon_pretty_print(const struct DaemonConfig* sc);

