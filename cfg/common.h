#ifndef __CFG_COMMON_H__
#define __CFG_COMMON_H__

#include <stdio.h>

#define CFG_PROC_OK                 0
#define CFG_PROC_IO                 -1
#define CFG_PROC_SYNTAX             -2
#define CFG_PROC_WRONG_SECTION      -10
#define CFG_PROC_KEY_BAD            -20
#define CFG_PROC_KEY_DUP            -21
#define CFG_PROC_VALUE_BAD          -30

typedef int (*cfg_proc_new_section)(const char* s);
typedef int (*cfg_proc_new_key_value)(const char* k, const char* v);

int cfg_proc_read(const char* fname, cfg_proc_new_section _new_sec, cfg_proc_new_key_value _new_kv);
unsigned cfg_proc_err_line_num();
unsigned cfg_proc_err_line_pos();

const char* cfg_proc_err_msg(int err);


#endif // __CFG_COMMON_H__

