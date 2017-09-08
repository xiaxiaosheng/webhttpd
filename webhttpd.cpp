#ifndef _WEBHTTPD_CPP_
#define _WEBHTTPD_CPP_


#include "webhttpd.h"
#include <cstdio>

namespace twebhttpd {



int HttpRequest::init(HttpInfoTrans* http_info) {
    std::string tmp, req_line;
    int len = WebHttpd::read_line(http_info->sock_, req_line);
    if(len < 0) return -1;
    std::vector<std::string> req_header = this->split_header(req_line.c_str(), (char*)" ");
    if(req_header.size() < 3) {
        return -1;
    }
    this->header.method = req_header[0];
    this->header.path = req_header[1];
    this->header.http_version = req_header[2];
    while(1) {
        len = WebHttpd::read_line(http_info->sock_, tmp);
        if(len <= 0) break;
        req_header = this->split_header(tmp.c_str(), (char*)" ");
        if(req_header.size() < 2) continue;
        this->header.add(req_header[0], req_header[1]);
    }
    if((int)this->header.method.find("POST") >= 0) {
        int length = atoi(this->header.get("Content-Length").c_str());
        WebHttpd::read_bytes(http_info->sock_, this->body_, length);
        puts(this->body_.c_str());
    }
    return 0;
}

std::vector<std::string> HttpRequest::split_header(const char* s, char* step) {
    std::vector<std::string> arr;
    arr.clear();
    if(NULL == s) return arr;
    std::string tmp;
    char* ptr;
    int index;
    while(1) {
        ptr = strstr(s, step);
        if(NULL == ptr) {
            tmp = s;
            arr.push_back(tmp);
            break;
        }
        *ptr = '\0';
        tmp = s;
        arr.push_back(tmp);
        s = ptr + strlen(step);
    }
    return arr;
}

HttpResponse::HttpResponse() {
    this->header.http_version = "HTTP/1.1";
    this->header.status = "OK";
    this->header.status_code = "200";
    this->header.add("Content-Type", "text/plain; charset=utf-8");
    this->body_ = "";
}

HttpResponse::~HttpResponse() {
    
}

void HttpResponse::init(HttpInfoTrans* http_info) {
    this->sock_ = http_info->sock_;
}

void HttpResponse::writer(const std::string& str) {
    this->body_ += str;
}

void HttpResponse::do_resp(const std::string& str) {
    this->body_ += str;
    this->do_resp();
}

void HttpResponse::do_resp() {
    char buff[BUFFSIZE];
    memset(buff, 0, sizeof(buff));
    sprintf(buff, "%s %s %s", this->header.http_version.c_str(), this->header.status_code.c_str(), this->header.status.c_str());
    this->send_line(buff);
    sprintf(buff, "Content-Length: %lu", this->body_.length());
    this->send_line(buff);
    for(std::map<std::string, std::string>::iterator iter = this->header.header_.begin(); iter != this->header.header_.end();iter++) {
        
        sprintf(buff, "%s: %s", iter->first.c_str(), iter->second.c_str());
        this->send_line(buff);
    }
    this->send_line((char*)"");
    memcpy(buff, this->body_.c_str(), this->body_.length() + 1);
    this->send_line(buff);
}

void HttpResponse::send_line(char buff[]) {
    if(strlen(buff) > 0) {
        send(this->sock_, buff, strlen(buff), 0);
        memset(buff, 0, BUFFSIZE);
    }
    send(this->sock_, "\r\n", 2, 0);
}

void HttpResponse::resp_error404() {
    this->header.status = "Not Found";
    this->header.status_code = "404";
    this->body_ = "404 page not found";
    this->do_resp();
}

WebHttpd::WebHttpd() {
    this->port_ = 80;
    this->server_addr_ = htonl(INADDR_ANY);
    this->handler_route_ = NULL;
    shutdown(this->sock_, SHUT_RDWR);
    close(this->sock_);
}

WebHttpd::~WebHttpd() {
    delete this->handler_route_;
}

int WebHttpd::read_line(int sock, std::string& line) {
    char c = 0;
    int len = 0;
    line.clear();
    while(1) {
        len = recv(sock, &c, 1, 0);
        if(len < 0) return -1;
        if(0 == len) break;
        if('\r' == c) continue;
        if('\n' == c) break;
        line += c;
    }
    return line.length();
}

int WebHttpd::read_bytes(int sock, std::string& line, int len) {
    if(len <= 0) return 0;
    char c = 0;
    char buffer[len + 1];
    memset(buffer, 0, sizeof(buffer));
    line.clear();
    int ll = recv(sock, buffer, len, 0);
    buffer[len] = '\0';
    line = buffer;
    return ll;
}

void* WebHttpd::response_handler(void* p) {
    if(NULL == p) {
        return NULL;
    }
    HttpInfoTrans* http_info = (HttpInfoTrans*)p;
    HttpRequest* req = new HttpRequest;
    bool keep_alive = true;
    while(keep_alive) {
        if(req->init(http_info) < 0) break;
        HttpResponse resp;
        resp.init(http_info);
        router::iterator iter;
        if(NULL != http_info->handler_route_)
            iter = http_info->handler_route_->find(req->header.path);
        if(NULL == http_info->handler_route_ || http_info->handler_route_->end() == iter) { // the router not exist
            printf("router not found, response 404\n");
            resp.resp_error404();
            continue;
        }
        HandlerFunction* serve = (HandlerFunction*)iter->second;
        // if not keep-alive
        if(!((req->header.http_version.find("1.1") && (int)req->header.get("Connection").find("close") < 0) || 
            (int)req->header.get("Connection").find("keep-alive") >= 0)) {
            keep_alive = false;
            resp.header.add("Connection", "close");
        }
        serve(req, resp);
    }
    shutdown(http_info->sock_, SHUT_RDWR);
    close(http_info->sock_);
    return NULL;
}

void WebHttpd::handle_func(std::string path, HandlerFunction* func) {
    if(NULL == this->handler_route_) {
        this->handler_route_ = new router;
    }
    (*this->handler_route_)[path] = func;
}

int WebHttpd::start_service() {
    this->server_sock_addr_.sin_family = AF_INET;
    this->server_sock_addr_.sin_addr.s_addr = this->server_addr_;
    this->server_sock_addr_.sin_port = htons(this->port_);
    if((this->sock_ = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("create socket error!\n");
        return -1;
    }
    if(bind(this->sock_, (struct sockaddr*)&this->server_sock_addr_, sizeof(struct sockaddr))) {
        printf("bind sock addr error!\n");
        return -1;
    }
    if(listen(this->sock_, SOMAXCONN) < 0) {
        printf("listen error!\n");
        return -1;
    }
    while(true) {
        try {
            socklen_t sin_size = sizeof(struct sockaddr_in);
            sockaddr_in* client_sock_addr = new sockaddr_in;
            int client_sock = 0;
            pthread_t pth_handler;
            if((client_sock = accept(this->sock_, (struct sockaddr*)client_sock_addr, &sin_size )) < 0) {
                printf("accept error!\n");
                continue;
            }
            HttpInfoTrans* http_info = new HttpInfoTrans;
            http_info->sock_ = client_sock;
            http_info->handler_route_ = this->handler_route_;
            http_info->client_addr_ = client_sock_addr;
            pthread_create(&pth_handler, NULL, WebHttpd::response_handler, (void*)http_info);
            pthread_detach(pth_handler);
        } catch(...) {

        }
    }
}

}

#endif