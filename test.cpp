#include <cstdio>
#include "webhttpd.h"
#include <cstring>

void helloworld(twebhttpd::HttpRequest* req, twebhttpd::HttpResponse resp) {
    resp.header.add("Content-Type", "application/json");
    resp.do_resp("{\"code\": 0, \"msg\": \"\"}");
}

int main() {
    twebhttpd::WebHttpd http;
    http.set_port(8080);
    http.handle_func("/hello", (twebhttpd::HandlerFunction*)helloworld);
    http.start_service();
}
