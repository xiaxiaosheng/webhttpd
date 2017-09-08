# webhttpd
a C++ http lib.
It's very easy to use.

example:
twebhttpd::WebHttpd http;
http.set_port(8080);

http.handle_func("/hello", (twebhttpd::HandlerFunction*)helloworld);

http.start_service();
