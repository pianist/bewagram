#include "call_button.h"
#include <aux/logger.h>
#include <evcurl/evcurl.h>

extern evcurl_processor_t* g_evcurl_proc;
extern struct DaemonConfig g_dcfg;

static void test_req_cb(evcurl_req_result_t* res)
{
    printf("Req tst DONE: %d\n", res->result);
    printf("    effective URL: %s\n", res->effective_url);
    printf("    %ld\n", res->response_code);
    printf("    Content-Type: %s\n", res->content_type ? res->content_type : "(none)");
}

static void call_button_cb(int gpio_num, char state)
{
    log_info("KNOPKA %s", state == '1' ? "otpustili" : "nazhali");

    if ('0' == state)
    {
        if (strlen(g_dcfg.button.http_GET_log) > 0)
        {
            log_info("sending log request: %s", g_dcfg.button.http_GET_log);
            evcurl_new_http_GET(g_evcurl_proc, g_dcfg.button.http_GET_log, test_req_cb);
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
        fprintf(stderr, "Can't open gpio %d: %s\n", dc->gpio.call_button, strerror(errno));
        return -1;
    }

    log_warn("init button gpio done");
    return 0;
}

