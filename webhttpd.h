#ifndef _WEBHTTPD_H_
#define _WEBHTTPD_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstdlib>
#include <set>

#define BUFFSIZE 256

namespace twebhttpd {

class WebHttpd;
class Header;
class HttpRequest;
class HttpResponse;


typedef  void*HandlerFunction(HttpRequest*, HttpResponse) ;
typedef std::map<std::string, HandlerFunction*> router;

class Header {
public:
    std::string method;
    std::string http_version;
    std::string path;
    std::string status;
    std::string status_code;

    Header() {
        this->header_.clear();
    }
    ~Header() {
        this->header_.clear();
    }

    inline std::string get(std::string key) {
        return this->header_[key];
    }

    inline void set(std::string key, std::string value) {
        this->header_[key] = value;
    }

    inline void add(std::string key, std::string value) {
        this->set(key, value);
    }
public:
    std::map<std::string, std::string> header_;
};

struct HttpInfoTrans {
    int sock_;
    router* handler_route_;
    sockaddr_in* client_addr_;
    HandlerFunction* static_file_handler;

    inline ~HttpInfoTrans() {
        delete this->client_addr_;
    }
};

class HttpRequest {
private:
    std::vector<std::string> split_header(const char*, char*);
    void parse_get_param(std::string);
    void parse_post_param(std::string);
    void parse_param(std::string);
public:
    int parse_request(HttpInfoTrans*);
    Header header;

private:
    int sock_;
    std::string body_;
};

class HttpResponse {
public:
    Header header;

public:
    HttpResponse();
    ~HttpResponse();
    void init(HttpRequest*, HttpInfoTrans*);

    void resp_error404();
    void resp_error500(){}
    void do_resp();
    void do_resp(const std::string&);
    void writer(const std::string&);
    void send_line(char* line);
private:
    int sock_;
    std::string body_;
};


struct FileHandler {
	std::string strip_prefix;
	std::string path;
};

class WebHttpd {
public:
    inline void set_port(int port) {
        this->port_ = port;
    }

    WebHttpd();
    ~WebHttpd();

    int start_service();
    int stop();

    static int read_line(int, std::string&);
    static int read_bytes(int, std::string&, int);
    void handle_func(std::string, HandlerFunction*);

public:
    HandlerFunction* static_file_handler_;
//    void* add_file_handler(std::string, std::string, std::string);

private:
    static void* response_handler(void*);

     int sock_;
    uint port_;
    u_int32_t server_addr_;
    sockaddr_in server_sock_addr_;
    static std::set<int> g_sock_sets;

    std::map<std::string, HandlerFunction*>* handler_route_;
    //static std::map<std::string, FileHandler>* file_handler_route_;
    static void stop(int);

};

}
#endif
