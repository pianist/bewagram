#ifndef __evcurl_h__
#define __evcurl_h__

#include <ev.h>
#include <curl/curl.h>


typedef struct evcurl_processor_s
{
    struct ev_loop *loop;
    struct ev_timer timer_event;
    CURLM *multi;
    int still_running;
} evcurl_processor_t;

evcurl_processor_t *evcurl_create(struct ev_loop* loop);
void evcurl_destroy(evcurl_processor_t* __evcurl_pi);


typedef struct evcurl_req_result_s
{
    CURLcode result;
    const char* effective_url;
    long response_code;
    const char* content_type;

    void* body;
    size_t sz_body;
} evcurl_req_result_t;

typedef void (*evcurl_req_done_cb)(evcurl_req_result_t* res);

CURLMcode evcurl_new_http_GET(evcurl_processor_t *mp, char *url, evcurl_req_done_cb _finish_cb);

typedef struct evcurl_upload_req_s
{
    long timeout;
    long connect_timeout;

    const char* url;
    const char* username;
    const char* password;

    void* buf;
    size_t sz_buf;
} evcurl_upload_req_t;

CURLMcode evcurl_new_UPLOAD(evcurl_processor_t *mp, const evcurl_upload_req_t* req, evcurl_req_done_cb _finish_cb);

#endif //__evcurl_h__

