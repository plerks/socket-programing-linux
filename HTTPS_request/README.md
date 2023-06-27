Improve the HTTP_request to HTTPS_request using [openssl](https://www.openssl.org/) library.

Implemented by simply creating a new thread to perform the request and call the callback function.

Default c lib does not have common data structures like String or Map. And I did't implement one myself. So I need to define buf length as an estimated upper bound or write some other compromised code.

Asynchronous I/O suits the requirement best. There are [aio](https://man7.org/linux/man-pages/man7/aio.7.html) functions in linux but I did't use them in the implementation.

## How to run
I'm under ubuntu. First, have openssl library installed by `sudo apt-get install libssl-dev=3.0.2-0ubuntu1.10`.

Need to ensure the server is running. You can run **HTTPS_server** or change the domain name in src/HttpsRequestDemo.c to some public domain name. Take an example of GET <https://www.baidu.com/index.html>, baidu did't close the connection after responding. Actually, it's a keep-alive connection and has "Connection: keep-alive" in the response header, so I need to use Content-Length header to tell if the client has received all the data and close the connection in the client side (see MyHttps.c threadRun_get() function), else my client implementation will keep pending.

1.Open **HTTPS_request** folder with VSCode and open src/HttpsRequestDemo.c to run.

2.Or
```
HTTPS_request> gcc src/HttpsRequestDemo.c lib/MyHttps.c -I include -lssl -lcrypto -o src/HttpsRequestDemo
HTTPS_request> ./src/HttpsRequestDemo
```

## About the HTTPS certificate verify.
By calling `SSL_CTX_load_verify_locations()` in lib/MyHttps.c, the certificate tls_files/server.crt that I created using openssl command line is trusted. But the domain name inner it is "gxytestserver.com", so the certificate check will fail, and as for requesting other public domain, the certificate check will also fail since I did't place their corresponding certificate files (see my comment in lib/MyHttps.c) in `SSL_CTX_load_verify_locations()` CApath.