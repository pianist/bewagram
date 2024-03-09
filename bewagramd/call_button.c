#include "call_button.h"
#include <aux/logger.h>

static void call_button_cb(int gpio_num, char state)
{
    log_info("KNOPKA %s", state == '1' ? "otpustili" : "nazhali");
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
        fprintf(stderr, "Can't open gpio %d: %s\n", dc->gpio.call_button, strerror(errno));
        return -1;
    }

    log_warn("init button gpio done");
    return 0;
}

