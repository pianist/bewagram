#ifndef __HITINY_SYS_H__
#define __HITINY_SYS_H__

int hitiny_MPI_SYS_Init();
void hitiny_MPI_SYS_Done();

int hitiny_sys_bind_VPSS_GROUP(int vpss_dev_id, int vpss_chn_id, int grp_id);
int hitiny_sys_unbind_VPSS_GROUP(int vpss_dev_id, int vpss_chn_id, int grp_id);

int hitiny_open_dev(const char* fname);

#endif // __HITINY_SYS_H__

