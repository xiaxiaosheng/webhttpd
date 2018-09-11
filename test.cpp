#include <cstdio>
#include "webhttpd.h"
#include <cstring>
#include <string.h>
#include <algorithm>
#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

const int MAX_PATH = 256;

void helloworld(twebhttpd::HttpRequest* req, twebhttpd::HttpResponse resp) {
    resp.header.add("Content-Type", "application/json");
    resp.do_resp("{\"code\": 0, \"msg\": \"\"}");
}

void staticFile(twebhttpd::HttpRequest* req, twebhttpd::HttpResponse resp) {
    std::string path = req->header.path;
    if(path[path.length()-1] == '/') {
        path += "index.html";
    }
    char curr_path[MAX_PATH];
    getcwd(curr_path, MAX_PATH);
    std::string curr_dir(curr_path);
    curr_dir += path;
    FILE *file = fopen((const char*)curr_dir.c_str(), "r");
    printf("request path is[%s]\n", curr_dir.c_str());
    if(NULL == file) {
        resp.resp_error404();
        return;
    }
    std::string file_body = "";
    while(!feof(file)) {
        char ch = fgetc(file);
        file_body += ch;
    }
    resp.header.add("Conteng-Type", "plain/text; charset=UTF-8");
    resp.do_resp(file_body);
}

void get_test() {
    char curr_path[MAX_PATH];
    #ifdef WIN32
        getcwd(curr_path, MAX_PATH);
    #else
        getcwd(curr_path, MAX_PATH);
    #endif
    printf("curr_path=[%s]\n", curr_path);
}

int main() {
    twebhttpd::WebHttpd http;
    http.set_port(8081);
    http.handle_func("/hello", (twebhttpd::HandlerFunction*)helloworld);
    http.static_file_handler_ = (twebhttpd::HandlerFunction*)staticFile;
    http.start_service();
}
