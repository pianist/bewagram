#ifndef __BEWAGRAMD_CALL_BUTTON_H__
#define __BEWAGRAMD_CALL_BUTTON_H__

#include "cfg.h"
#include <evgpio/evgpio.h>
#include <ev.h>

int init_button_evloop(struct ev_loop* loop, const struct DaemonConfig* dc);


#endif //__BEWAGRAMD_CALL_BUTTON_H__

