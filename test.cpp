#include <cstdio>
#include "webhttpd.h"
#include <cstring>

void helloworld(twebhttpd::HttpRequest* req, twebhttpd::HttpResponse resp) {
    resp.header.add("Content-Type", "application/json");
    resp.do_resp("{\"code\": 0, \"msg\": \"\"}");
}

int main() {
    twebhttpd::Server server;
    server.set_port(8080);
    server.handle_func("/hello", (twebhttpd::HandlerFunction*)helloworld);
    server.start_service();
}
