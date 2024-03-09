#include <evgpio/evgpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <ev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_GPIO_WATCH 8

static int __CNT = 0;

static void timeout_cb(struct ev_loop *loop, ev_timer* w, int revents)
{
    fprintf(stderr, "timer!\n");
    evgpio_watcher_print_list();
    ev_break(EV_A_ EVBREAK_ONE);

    //if (__CNT == 3) evgpio_watcher_remove(loop, 3);
    //if (__CNT == 4) evgpio_watcher_remove(loop, 4);
    //if (__CNT > 10) { ev_timer_stop(loop, w); }  else ev_timer_again(loop, w);

    ev_timer_again(loop, w);
    __CNT++;
}

void my_gpio_cb(int gpio_num, char state)
{
    printf("GPIO %d to %c\n", gpio_num, state);
}

int test_gpios(struct ev_loop* loop, int gpio_cnt, int* gpios)
{
    for (int i = 0; i < gpio_cnt; ++i)
    {
        int ret = evgpio_watcher_init(loop, gpios[i], my_gpio_cb);
        if (ret < 0)
        {
            fprintf(stderr, "Can't open gpio %d: %s\n", gpios[i], strerror(errno));
            return -1;
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    int wait_timeout = 1;
    int gpio_nums[MAX_GPIO_WATCH] = { 0 };
    int gpio_cnt = 0;

    if (argc >= MAX_GPIO_WATCH)
    {
        fprintf(stderr, "Too many arguments\n");
        return -1;
    }

    for (int i = 1; i < argc; ++i)
    {
        int gpio_num = atoi(argv[i]);
        if (!gpio_num)
        {
            fprintf(stderr, "Wrong argument '%s'\n", argv[i]);
            return -1;
        }
        gpio_nums[gpio_cnt] = gpio_num;
        gpio_cnt++;
    }

    if (!gpio_cnt)
    {
        fprintf(stderr, "Usage: ./test_watcher_gpio gpioA [gpioB gpioC ...]\n");
        return -1;
    }

    struct ev_loop* loop = ev_loop_new(EVBACKEND_EPOLL | EVFLAG_NOENV);

    int ret = test_gpios(loop, gpio_cnt, gpio_nums);
    ev_timer timeout_watcher;
    ev_timer_init(&timeout_watcher, timeout_cb, 1., 0.);    
    timeout_watcher.repeat = 2.;
    ev_timer_start(loop, &timeout_watcher);

    do
    {
        ret = ev_run(loop, 0);
    }
    while (ret);

    return ret;
}

