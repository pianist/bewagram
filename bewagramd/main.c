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
#include <hitiny/hitiny_sys.h>
#include <hitiny/hitiny_aio.h>
#include <hitiny/hitiny_venc.h>

#include "call_button.h"
#include "snap.h"

int stop_flag = 0;
evcurl_processor_t* g_evcurl_proc = 0;
struct DaemonConfig g_dcfg;

void action_on_signal(int signum)
{
    log_info("STOP");
    stop_flag = 1;
}

void print_cfg_error(int ret)
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
    int no_deamon_mode = 0;
    int cfg_default = 1;

    if (argc > 1)
    {
        if (!strcmp(argv[1], "help"))
        {
            fprintf(stderr, "Usage: bewagramd [--no-daemon] config.ini");
            return -1;
        }

        if (!strcmp(argv[1], "--no-daemon"))
        {
            no_deamon_mode = 1;
            if (argc > 2) cfg_default = 0;
        }
        
    }

    const char* cfg_fname = (cfg_default) ? "/etc/bewagramd.ini" : argv[argc - 1];
    log_info("Starting, loading config from %s", cfg_fname);

    int ret = cfg_daemon_read(cfg_fname, &g_dcfg);
    if (ret < 0)
    {
        print_cfg_error(ret);
        return ret;
    }

    if (!no_deamon_mode)
    {
        char buf[128];
        snprintf(buf, 128, "/tmp/bewagram.log");
        log_info("Daemonize and log to %s", buf);
        log_create(buf, LOG_info);

        if (fork()) exit(0);
    }

    ret = hitiny_MPI_SYS_Init();
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_Init() failed: 0x%X", ret);
        return ret;
    }

    signal(SIGINT, action_on_signal);
    signal(SIGTERM, action_on_signal);

    struct ev_loop* loop = ev_loop_new(EVBACKEND_EPOLL | EVFLAG_NOENV);
    g_evcurl_proc = evcurl_create(loop);

    init_button_evloop(loop, &g_dcfg);

    hitiny_MPI_VENC_Init();
    ret = init_snap_machine(loop, &g_dcfg);
    if (ret)
    {
        log_crit("SNAP machine critical (0x%x), exit", ret);
        hitiny_MPI_VENC_Done();
        hitiny_MPI_SYS_Done();
        exit(-1);
    }

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

    done_snap_machine(loop, &g_dcfg);
    hitiny_MPI_VENC_Done();

    hitiny_MPI_SYS_Done();

    log_info("The end!");

    return 0;
}

