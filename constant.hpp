#ifndef CONSTANT_HPP_
#define CONSTANT_HPP_

const int BUF_SIZE = 1024;

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
