#include "evgpio.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_PATH_LEN 256

typedef struct __evgpio_watcher_t
{
    ev_io _ev_io;
    int gpio_num;
    evgpio_watcher_cb cb;
    struct __evgpio_watcher_t* next;
} evgpio_watcher_s;

static void __gpio_cb(struct ev_loop *loop, ev_io* _w, int revents)
{
    evgpio_watcher_s* w = (evgpio_watcher_s*)_w;

    //fprintf(stderr, "event on fd = %d (GPIO %d): 0x%x\n", w->_ev_io.fd, w->gpio_num, revents);

    char x = '?';
    int r1 = lseek(w->_ev_io.fd, 0, SEEK_SET);
    int r2 = read(w->_ev_io.fd, &x, sizeof(char));
    //fprintf(stderr, "\tr1=%d, r2=%d\n", r1, r2);

    if (r1 < 0 || r2 < 0)
    {
        evgpio_watcher_remove(loop, w->gpio_num);
        return;
    }

    w->cb(w->gpio_num, x);
}

static evgpio_watcher_s* __evgpio_watcher_list = 0;

void evgpio_watcher_print_list()
{
    evgpio_watcher_s* w = __evgpio_watcher_list;
    while (w)
    {
        printf("%d;", w->gpio_num);
        w = w->next;
    }
    printf("\n");
}

void evgpio_watcher_remove(struct ev_loop* loop, int gpio_num)
{
    evgpio_watcher_s* w = __evgpio_watcher_list;
    evgpio_watcher_s* _prev = 0;

    while (w)
    {
        if (w->gpio_num == gpio_num)
        {
            ev_io_stop(loop, &(w->_ev_io));
            close(w->_ev_io.fd);
            if (_prev)
            {
                _prev->next = w->next;
            }
            else
            {
                __evgpio_watcher_list = w->next;
            }
            break;
        }
        _prev = w;
        w = w->next;
        free(w);
    }
}

int evgpio_watcher_init(struct ev_loop* loop, int gpio_num, evgpio_watcher_cb _cb)
{
    evgpio_watcher_s* w = (evgpio_watcher_s*)malloc(sizeof(evgpio_watcher_s));

    if (!w) return -1;

    char filename[MAX_PATH_LEN];
    int fd;

    char buf[8];
    int buf_len = snprintf(buf, 8, "%d", gpio_num);
    if (buf_len <= 0) return -1; 

    snprintf(filename, MAX_PATH_LEN, "/sys/class/gpio/export", gpio_num);
    fd = open(filename, O_WRONLY);
    if (fd < 0)
    {
        free(w);
        return -1;
    }
    write(fd, buf, buf_len);
    close(fd);

    snprintf(filename, MAX_PATH_LEN, "/sys/class/gpio/gpio%d/direction", gpio_num);
    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        free(w);
        return -1;
    }
    buf_len = snprintf(buf, 8, "in");
    write(fd, buf, buf_len);
    close(fd);

    snprintf(filename, MAX_PATH_LEN, "/sys/class/gpio/gpio%d/edge", gpio_num);
    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        free(w);
        return -1;
    }
    buf_len = snprintf(buf, 8, "both");
    write(fd, buf, buf_len);
    close(fd);

    snprintf(filename, MAX_PATH_LEN, "/sys/class/gpio/gpio%d/value", gpio_num);
    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        free(w);
        return -1;
    }

    ev_io_init(&(w->_ev_io), __gpio_cb, fd, EV_PRI);
    w->gpio_num = gpio_num;
    w->cb = _cb;
    w->next = __evgpio_watcher_list;
    __evgpio_watcher_list = w;

    ev_io_start(loop, &(w->_ev_io));

    return 0;
}

