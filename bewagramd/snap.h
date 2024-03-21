#ifndef __BEWAGRAMD_SNAP_H__
#define __BEWAGRAMD_SNAP_H__

#include "cfg.h"
#include <ev.h>

int init_snap_machine(struct ev_loop* loop, const struct DaemonConfig* dc);
int done_snap_machine(const struct DaemonConfig* dc);


#endif

