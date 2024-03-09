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
};

struct DaemonConfig {
    struct Device_GPIOs gpio;
    struct ButtonConfig button;
};

int cfg_daemon_read(const char* fname, struct DaemonConfig* sc);
const char* cfg_daemon_read_error_key();
const char* cfg_daemon_read_error_value();

void cfg_daemon_pretty_print(const struct DaemonConfig* sc);

