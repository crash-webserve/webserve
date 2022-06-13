#ifndef CONSTANT_HPP_
#define CONSTANT_HPP_

const int BUF_SIZE = 0x1 << 10;
const int LISTEN_BACKLOG = 40;
const int DEFAULT_CLIENT_MAX_BODY_SIZE = 100000;
const std::string REDIRECT_PATH = "/Users/mike2ox/Project/webserve/html/redirect.html";
const std::string DEFAULT_CONF_PATH = "./conf/sample.conf";

#define SAMPLE_RESPONSE "HTTP/1.1 200 OK\r\n\
Content-Length: 365\r\n\
\r\n\
<!DOCTYPE html>\r\n\
<html>\r\n\
<head>\r\n\
<title>Welcome to nginx!</title>\r\n\
<style>\r\n\
html { color-scheme: light dark; }\r\n\
body { width: 35em; margin: 0 auto;\r\n\
font-family: Tahoma, Verdana, Arial, sans-serif; }\r\n\
</style>\r\n\
</head>\r\n\
<body>\r\n\
<h1>Welcome to nginx!</h1>\r\n\
\r\n\
<p><em>Thank you for using nginx.</em></p>\r\n\
</body>\r\n\
</html>\r\n\
\r\n\
\r\n"

#endif  // CONSTANT_HPP_
