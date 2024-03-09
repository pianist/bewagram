#include "common.h"
#include <stdio.h>
#include <string.h>

static unsigned __cfg_proc_line_num = 0;
static unsigned __cfg_proc_line_pos = 0;

unsigned cfg_proc_err_line_num()
{
    return __cfg_proc_line_num;
}

unsigned cfg_proc_err_line_pos()
{
    return __cfg_proc_line_pos;
}

#define CASE_ERR_MSG(err_code) case err_code: return #err_code;

const char* cfg_proc_err_msg(int err)
{
    switch (err)
    {
        CASE_ERR_MSG(CFG_PROC_IO)
        CASE_ERR_MSG(CFG_PROC_SYNTAX)
        CASE_ERR_MSG(CFG_PROC_WRONG_SECTION)
        CASE_ERR_MSG(CFG_PROC_KEY_BAD)
        CASE_ERR_MSG(CFG_PROC_KEY_DUP)
        CASE_ERR_MSG(CFG_PROC_VALUE_BAD)
        default: return "UNKNOWN";
    }
}

static const char* __space_chars = " \t\n\r";
static const char* __allowed_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_0123456789";
static const char* __allowed_chars_values = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_0123456789/.|";

int cfg_proc_read(const char* fname, cfg_proc_new_section _new_sec, cfg_proc_new_key_value _new_kv)
{
    __cfg_proc_line_num = 0;
    __cfg_proc_line_pos = 0;

    FILE* f = fopen(fname, "r");
    if (!f) return -1;

    char buf[1024];
    __cfg_proc_line_num = 0;
    while (fgets(buf, 1024, f))
    {
        __cfg_proc_line_num++;

        char* p = buf;
        while (*p && strchr(__space_chars, *p)) ++p;

        if (!*p) continue;
        if (*p == ';') continue;
        if (*p == '#') continue;

        if ('[' == *p)
        {
            p++;
            char* sec_name = p;
            while (*p && strchr(__allowed_chars, *p)) ++p;
            if (']' == *p)
            {
                *p = 0;
                if (p == sec_name)
                {
                    fclose(f);
                    __cfg_proc_line_pos = p - buf + 1;
                    return CFG_PROC_SYNTAX;
                }
                int r = _new_sec(sec_name);
                if (r < 0)
                {
                    fclose(f);
                    __cfg_proc_line_pos = sec_name - buf + 1;
                    return r;
                }
                p++;
            }
            else
            {
                fclose(f);
                __cfg_proc_line_pos = p - buf + 1;
                return CFG_PROC_SYNTAX;
            }

            while (*p && strchr(__space_chars, *p)) ++p;
            if (!*p) continue;
            if (*p == ';') continue;
            if (*p == '#') continue;

            fclose(f);
            __cfg_proc_line_pos = p - buf + 1;
            return CFG_PROC_SYNTAX;
        }

        char* key_name = p;
        while (*p && strchr(__allowed_chars, *p)) ++p;

        char* key_name_end = p;
        while (*p && strchr(__space_chars, *p)) ++p;

        if (*p != '=')
        {
            fclose(f);
            __cfg_proc_line_pos = p - buf + 1;
            return CFG_PROC_SYNTAX;
        }

        *key_name_end = 0;
        p++;
        while (*p && strchr(__space_chars, *p)) ++p;

        char* val = p;
        char* val_end = val;

        if (('"' == *p) || ('\'' == *p) || ('`' == *p))
        {
            char open_quot_char = *p;

            p++;
            val++;

            while (*p && (*p != open_quot_char)) ++p;

            if (*p != open_quot_char)
            {
                fclose(f);
                __cfg_proc_line_pos = p - buf + 1;
                return CFG_PROC_SYNTAX;
            }

            val_end = p;
            p++;
        }
        else
        {
            while (*p && strchr(__allowed_chars_values, *p)) ++p;
            val_end = p;
        }

        while (*p && strchr(__space_chars, *p)) ++p;
        if (*p && (*p != ';') && (*p != '#'))
        {
            fclose(f);
            __cfg_proc_line_pos = p - buf + 1;
            return CFG_PROC_SYNTAX;
        }

        *val_end = 0;

        int r = _new_kv(key_name, val);
        if (r < 0)
        {
            fclose(f);
            __cfg_proc_line_pos = p - buf + 1;
            return r;
        }
    }

    fclose(f);

    return 0;
}

