#include "evcurl.h"
#include <stdlib.h>

typedef struct evcurl_http_req_info_s
{
    CURL* easy;
    evcurl_processor_t* mp;
    evcurl_req_done_cb finish_cb;
    void* src_req_data;
    evcurl_req_result_t result;
} evcurl_http_req_info_t;


typedef struct evcurl_sock_info_s
{
    curl_socket_t sockfd;
    CURL *easy;
    int action;
    long timeout;
    struct ev_io ev;
    int evset;
    evcurl_processor_t *mp;
} evcurl_sock_info_t;

static void evcurl_check_multi_info(evcurl_processor_t *mp)
{
    char *eff_url;
    CURLMsg *msg;
    int msgs_left;
    evcurl_http_req_info_t *conn;
    CURL *easy;
    CURLcode res;

    while ((msg = curl_multi_info_read(mp->multi, &msgs_left)))
    {
        if (msg->msg == CURLMSG_DONE)
        {
            easy = msg->easy_handle;
            res = msg->data.result;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &conn->result.effective_url);
            curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &conn->result.response_code);
            curl_easy_getinfo(easy, CURLINFO_CONTENT_TYPE, &conn->result.content_type);


            conn->result.result = msg->data.result;
            if (conn->finish_cb) conn->finish_cb(&conn->result, conn->src_req_data);

            curl_multi_remove_handle(mp->multi, easy);
            curl_easy_cleanup(easy);

            if (conn->result.body) free(conn->result.body);
            free(conn);
        }
    }
}

static void __evcurl_event_cb(EV_P_ struct ev_io *w, int revents)
{
    evcurl_processor_t *mp = (evcurl_processor_t*)w->data;
    CURLMcode rc;

    int action = ((revents & EV_READ)  ? CURL_POLL_IN  : 0) |
                 ((revents & EV_WRITE) ? CURL_POLL_OUT : 0);

    rc = curl_multi_socket_action(mp->multi, w->fd, action, &mp->still_running);

    evcurl_check_multi_info(mp);
    if (mp->still_running <= 0)
    {
        ev_timer_stop(mp->loop, &mp->timer_event);
    }
}

static void __evcurl_timer_cb(EV_P_ struct ev_timer *w, int revents)
{
    evcurl_processor_t *g = (evcurl_processor_t*)w->data;

    CURLMcode rc;
    rc = curl_multi_socket_action(g->multi, CURL_SOCKET_TIMEOUT, 0, &g->still_running);

    evcurl_check_multi_info(g);
}

static int __evcurl_multi_timer_cb(CURLM *multi, long timeout_ms, evcurl_processor_t *mp)
{
    ev_timer_stop(mp->loop, &(mp->timer_event));

    if (timeout_ms >= 0)
    {
        /* -1 means delete, other values are timeout times in milliseconds */
        double t = timeout_ms / 1000;
        ev_timer_init(&mp->timer_event, __evcurl_timer_cb, t, 0.);
        ev_timer_start(mp->loop, &mp->timer_event);
    }
    return 0;
}

static void setsock(evcurl_sock_info_t *f, curl_socket_t s, CURL *e, int act, evcurl_processor_t *mp)
{
    int kind = ((act & CURL_POLL_IN) ? EV_READ : 0) |
               ((act & CURL_POLL_OUT) ? EV_WRITE : 0);

    f->sockfd = s;
    f->action = act;
    f->easy = e;

    if (f->evset) ev_io_stop(mp->loop, &f->ev);
    ev_io_init(&f->ev, __evcurl_event_cb, f->sockfd, kind);

    f->ev.data = mp;
    f->evset = 1;
    ev_io_start(mp->loop, &f->ev);
}

static void addsock(curl_socket_t s, CURL *easy, int action, evcurl_processor_t *mp)
{
    evcurl_sock_info_t *fdp = calloc(1, sizeof(evcurl_sock_info_t));

    fdp->mp = mp;
    setsock(fdp, s, easy, action, mp);
    curl_multi_assign(mp->multi, s, fdp);
}

static void remsock(evcurl_sock_info_t *f, evcurl_processor_t *mp)
{
    if (f)
    {
        if (f->evset) ev_io_stop(mp->loop, &f->ev);
        free(f);
    }
}

static int __evcurl_sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp)
{
    evcurl_processor_t *g = (evcurl_processor_t*)cbp;
    evcurl_sock_info_t *fdp = (evcurl_sock_info_t*)sockp;

    if (what == CURL_POLL_REMOVE)
    {
        remsock(fdp, g);
    }
    else
    {
        if (!fdp)
        {
            addsock(s, e, what, g);
        }
        else
        {
            setsock(fdp, s, e, what, g);
        }
    }

    return 0;
}

evcurl_processor_t *evcurl_create(struct ev_loop* loop)
{
    evcurl_processor_t* __evcurl_pi = (evcurl_processor_t*)malloc(sizeof(evcurl_processor_t));

    memset(__evcurl_pi,  0, sizeof(evcurl_processor_t));

    __evcurl_pi->loop = loop;
    __evcurl_pi->multi = curl_multi_init();

    ev_timer_init(&(__evcurl_pi->timer_event), __evcurl_timer_cb, 0.01, 0.);
    __evcurl_pi->timer_event.data = __evcurl_pi;

    curl_multi_setopt(__evcurl_pi->multi, CURLMOPT_SOCKETFUNCTION, __evcurl_sock_cb);
    curl_multi_setopt(__evcurl_pi->multi, CURLMOPT_SOCKETDATA, __evcurl_pi);
    curl_multi_setopt(__evcurl_pi->multi, CURLMOPT_TIMERFUNCTION, __evcurl_multi_timer_cb);
    curl_multi_setopt(__evcurl_pi->multi, CURLMOPT_TIMERDATA, __evcurl_pi);

    return __evcurl_pi;
}

void evcurl_destroy(evcurl_processor_t* __evcurl_pi)
{
    curl_multi_cleanup(__evcurl_pi->multi);
    free(__evcurl_pi);
}

///////////////////////////////////////////////////////////////

static size_t __simple_body_write_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    evcurl_http_req_info_t *conn = (evcurl_http_req_info_t*)data;

    void* nbody = realloc(conn->result.body, conn->result.sz_body + realsize);
    if (!nbody) return 0;

    conn->result.body = nbody;

    memcpy((char*)nbody + conn->result.sz_body, ptr, realsize);
    conn->result.sz_body += realsize;

    return realsize;
}

CURLMcode evcurl_new_http_GET(evcurl_processor_t *mp, char *url, evcurl_req_done_cb _finish_cb)
{
    evcurl_http_req_info_t *conn;
    CURLMcode rc;

    conn = calloc(1, sizeof(evcurl_http_req_info_t));

    conn->easy = curl_easy_init();
    if (!conn->easy) return CURLM_OUT_OF_MEMORY;

    conn->mp = mp;
    conn->finish_cb = _finish_cb;

    curl_easy_setopt(conn->easy, CURLOPT_URL, url);

    curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(conn->easy, CURLOPT_MAXREDIRS, 3);

    curl_easy_setopt(conn->easy, CURLOPT_TIMEOUT, 30);
    curl_easy_setopt(conn->easy, CURLOPT_CONNECTTIMEOUT, 5);

//    curl_easy_setopt(conn->easy, CURLOPT_HEADERFUNCTION, __XXX_header_cb);
//    curl_easy_setopt(conn->easy, CURLOPT_HEADERDATA, conn);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, __simple_body_write_cb);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, conn);

    curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);

    rc = curl_multi_add_handle(mp->multi, conn->easy);
    return rc;
}

static size_t __upload_data_read_cb(char *buffer, size_t size, size_t nitems, void *userdata)
{
    evcurl_upload_req_t* req = (evcurl_upload_req_t*)userdata;

    size_t cpy_sz = size * nitems;
    size_t cpy_left = req->sz_buf - ((char*)req->ptr - (char*)req->buf);

    if (cpy_left < cpy_sz) cpy_sz = cpy_left;

    memcpy(buffer, req->ptr, cpy_sz);
    req->ptr = (char*)req->ptr + cpy_sz;

    return cpy_sz;
}

CURLMcode evcurl_new_UPLOAD(evcurl_processor_t *mp, evcurl_upload_req_t* req, evcurl_req_done_cb _finish_cb)
{
    evcurl_http_req_info_t *conn;
    CURLMcode rc;

    conn = calloc(1, sizeof(evcurl_http_req_info_t));

    conn->easy = curl_easy_init();
    if (!conn->easy) return CURLM_OUT_OF_MEMORY;

    conn->mp = mp;
    conn->finish_cb = _finish_cb;
    conn->src_req_data = req;

    curl_easy_setopt(conn->easy, CURLOPT_URL, req->url);

    curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(conn->easy, CURLOPT_MAXREDIRS, 3);

    curl_easy_setopt(conn->easy, CURLOPT_TIMEOUT, 30);
    curl_easy_setopt(conn->easy, CURLOPT_CONNECTTIMEOUT, 5);

    curl_easy_setopt(conn->easy, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_INFILESIZE, req->sz_buf);
    curl_easy_setopt(conn->easy, CURLOPT_READFUNCTION, __upload_data_read_cb);

    req->ptr = req->buf; // start READ from buf
    curl_easy_setopt(conn->easy, CURLOPT_READDATA, req);

    curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, __simple_body_write_cb);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, conn);

    curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);

    rc = curl_multi_add_handle(mp->multi, conn->easy);
    return rc;
}


