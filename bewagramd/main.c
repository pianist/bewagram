#include <evgpio/evgpio.h>
#include <evcurl/evcurl.h>
#include <ev.h>
#include "cfg.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <unistd.h>
#include <aux/logger.h>
#include "call_button.h"

int stop_flag = 0;
evcurl_processor_t* g_evcurl_proc = 0;
struct DaemonConfig g_dcfg;

void action_on_signal(int signum)
{
    log_info("STOP");
    stop_flag = 1;
}

void print_error(int ret)
{
    if (-1 == ret)
    {
        log_error("I/O error, %d: %s", errno, strerror(errno));
    }
    else
    {
        log_error("Error %s(%d) at line %u, pos %u", cfg_proc_err_msg(ret), ret, cfg_proc_err_line_num(), cfg_proc_err_line_pos());
        if (ret == CFG_PROC_WRONG_SECTION)
        {
            log_error("Wrong section name [%s]", cfg_daemon_read_error_value());
        }
        else if (ret == CFG_PROC_KEY_BAD)
        {
            log_error("Wrong key \"%s\"", cfg_daemon_read_error_key());
        }
        else if (ret == CFG_PROC_VALUE_BAD)
        {
            log_error("Wrong value \"%s\" for %s", cfg_daemon_read_error_value(), cfg_daemon_read_error_key());
        }
    }
}

static void timeout_cb(struct ev_loop *loop, ev_timer* w, int revents)
{
    //fprintf(stderr, "DBG: active gpio watchers: ");
    //evgpio_watcher_print_list();
    ev_break(EV_A_ EVBREAK_ONE);

    ev_timer_again(loop, w);
}

int main(int argc, char** argv)
{
    if (argc > 1 && !strcmp(argv[1], "help"))
    {
        fprintf(stderr, "Usage: bewagramd config.ini");
        return -1;
    }

    const char* cfg_fname = (argc > 1) ? argv[1] : "/etc/bewagramd.ini";
    log_info("Starting, loading config from %s", cfg_fname);

    int ret = cfg_daemon_read(cfg_fname, &g_dcfg);
    if (ret < 0)
    {
        print_error(ret);
        return ret;
    }

    signal(SIGINT, action_on_signal);

    struct ev_loop* loop = ev_loop_new(EVBACKEND_EPOLL | EVFLAG_NOENV);
    g_evcurl_proc = evcurl_create(loop);

    init_button_evloop(loop, &g_dcfg);

    ev_timer timeout_watcher;
    ev_timer_init(&timeout_watcher, timeout_cb, 1., 0.);
    timeout_watcher.repeat = 2.;
    ev_timer_start(loop, &timeout_watcher);

    do
    {
        ret = ev_run(loop, 0);
        if (stop_flag) break;
    }
    while (ret);

    // wait all threads
    evcurl_destroy(g_evcurl_proc);
    g_evcurl_proc = 0;

    log_info("The end!");

    return 0;
}

