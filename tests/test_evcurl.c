#include <evcurl/evcurl.h>

void req_end_cb(evcurl_req_result_t* res, void* src_req_data)
{
    printf("Req DONE: %d\n", res->result);
    printf("    effective URL: %s\n", res->effective_url);
    printf("    %ld\n", res->response_code);
    printf("    Content-Type: %s\n", res->content_type ? res->content_type : "(none)");

    if (!res->sz_body) return;

    printf("\n\n%.*s\n--------------------------------------------------------------\n", (int)res->sz_body, (const char*)res->body);
}

void PUT_req_end_cb(evcurl_req_result_t* res, void* src_req_data)
{
    printf("PUT Req DONE: %d\n", res->result);
    struct evcurl_upload_req_s* put_req = (struct evcurl_upload_req_s*)src_req_data;
    printf("    upload data sz was %u\n", put_req->sz_buf);
    printf("    effective URL: %s\n", res->effective_url);
    printf("    %ld\n", res->response_code);
    printf("    Content-Type: %s\n", res->content_type ? res->content_type : "(none)");

    if (!res->sz_body) return;

    printf("\n\n%.*s\n--------------------------------------------------------------\n", (int)res->sz_body, (const char*)res->body);
}


int main(int argc, char **argv)
{
    struct ev_loop *loop = ev_loop_new(EVBACKEND_POLL | EVFLAG_NOENV);

    evcurl_processor_t* _p = evcurl_create(loop);

    evcurl_new_http_GET(_p, "http://192.168.88.1/", req_end_cb);
    evcurl_new_http_GET(_p, "http://192.168.88.222/", req_end_cb);
    evcurl_new_http_GET(_p, "https://example.com/", req_end_cb);

    struct evcurl_upload_req_s put_req;
    memset(&put_req, 0, sizeof(struct evcurl_upload_req_s));
    put_req.buf = "qweqweqwe";
    put_req.sz_buf = 9;
    put_req.ptr = put_req.buf;
    put_req.url = "http://example.com/upload";
    evcurl_new_UPLOAD(_p, &put_req, PUT_req_end_cb);

    ev_loop(_p->loop, 0);

    evcurl_destroy(_p);

    ev_loop_destroy(loop);
}

