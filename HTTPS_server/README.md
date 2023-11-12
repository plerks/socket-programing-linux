Improve the HTTP_server to HTTPS_server using [openssl](https://www.openssl.org/) library. Able to serve static files and Implemented basic function that user can configure url with its corresponding response.

The function is limited and not easy to use, far less than formal framework. And code is not mature enough, I did't write adequate error processing code and not suitable for handling abnormal situation.

There are [aio](https://man7.org/linux/man-pages/man7/aio.7.html) functions in linux but I did't use them in the implementation. For the [windows](https://github.com/plerks/socket-programing-windows) code I used asynchrous I/O.

Default c lib does not have common data structures like String or Map. And I did't implement one myself. So I need to define buf length as an estimated upper bound or write some other compromised code.

The book's chapter24 demonstrated how to code a basic HTTP server. But it's quite basic, just accept, create new thread to read, resolve and write, without using any concurrent mode introduced in the book before. And the author mentioned that because for normal HTTP protocol, the server just close the connetion after responsing (short connection), no time for IOCP/epoll to make much effect, so using IOCP/epoll won't bring much improvement. Despite that, I used epoll to implement the server (originate from chapter17/echo_EPLTserv.c).

Use epoll to monitor socket file descriptors, accept client connection request, put client fds into a queue (with a mutex to guarantee thread safety) when there gets data to read. And mutiple worker threads retrieve client fd and handle the request. This is a typical producer-consumer problem. My code is rough and could be optimized with better code, for example, Java's ArrayBlockingQueue. And I saw [this](https://www.bilibili.com/video/BV1o84y1D7RV/) video, it offered another idea, each consumer has their own queue and the main thread delivers fds in turn to worker threads' queue. Each consumer has their own queue enables consumers to be independent, without trying to retrieve fd from the same queue. Despite that it may cause certain worker threads to be idle when it receives mostly easy-to-handle client request while others the opposite. This problem requires better load balance algorithm.

## How to run
I'm under ubuntu. First, have openssl library installed by `sudo apt-get install libssl-dev=3.0.2-0ubuntu1.10`.

In linux, to let the server listen at port 80 needs sudo privilege so I changed to port to 44300. If want to serve at 443, you can change the port number in DemoApplication.c and compile to run.

1.Open **HTTPS_server** folder with VSCode and open src/DemoApplication.c to run.

2.Or
```
HTTPS_server> gcc src/DemoApplication.c lib/Handlers.c lib/Server.c -I include -lssl -lcrypto -o src/DemoApplication
HTTPS_server> ./src/DemoApplication
```

## How to see the effect
I have put some static files under resources and configured some urls' response.

* visit <https://localhost:44300/index.html> for static file index.html.
* visit <https://localhost:44300/getdemo> for self-configured GET response.
* POST <https://localhost:44300/postdemo> for self-configured POST response. (With Postman or something else, only support when the request indicates its request body length by `Content-Length`)
* other requests for errorpage

## Install openssl
1.install openssl library.

I'm under ubuntu, so openssl library can be installed by `sudo apt-get install libssl-dev`. By `sudo apt list --installed | grep libssl`, my installed openssl library version is 3.0.2-0ubuntu1.10. So it's better to install the same version of mine by `sudo apt-get install libssl-dev=3.0.2-0ubuntu1.10`.

2.install openssl executable.

Run `sudo apt-get install openssl` to install openssl executable. My `openssl version`: "OpenSSL 3.0.2 15 Mar 2022 (Library: OpenSSL 3.0.2 15 Mar 2022)".

## Generate crt and key file
Referring to [this](https://ningyu1.github.io/site/post/51-ssl-cert/), run:

1. run `openssl genrsa -out server.key 2048` to generate private key
2. run `openssl req -new -key server.key -out server.csr` to generate the .csr file. Fill in your domain name in the common name field in the terminal. I tested that openssl permits to generate a certificate with domain name localhost.
3. run `openssl x509 -req -in server.csr -out server.crt -signkey server.key -days 36500` to generate the .crt file

I have placed the .crt and .key file at the tls_files folder.

## How the HTTP request/response body end is detected?

First of all, the server and client can both easily know the end of headers by detecting the emptyLine (detecting "\r\n\r\n").

HTTP protocol's typical process: client send -> server response -> server close the connection. (not considering HTTP persistent connection)

So the client can know that the server has sent all the response content by detect the close of connection (read() returns 0). Or the response may also have `Content-Length` header (usually no as far as I see), the client can detect the end by it. For this my implementation, I have't written a `Content-Length` header in the response, and if in Server.c the `close(client_sock);` is commented, Chrome browser just keeps waiting if visit <https://localhost:44300/index.html>, thus proved.

And as for how server detects the end of request body. If it's a GET request, then it's supposed to have no body (thus done). And if it's a POST request, see [this](https://stackoverflow.com/questions/4824451/detect-end-of-http-request-body), the request headers should indicate the body length by `Content-Length`(for fixed length) or `Transfer-Encoding: Chunked`(for uncertain length).

## Understanding of asynchronous I/O
The introduction of asynchronous I/O is supposed to be based on such fact that I/O is usually slower than common actions, especially socket I/O that requires geographic transmission.

So, when the thread changes synchronous I/O to asynchronous I/O, it has the chance to perform other actions since returning from the I/O functions immediately, instead of pending on the slow I/O functions. And the actual I/O is performed by more basic layers. And the finish of asynchronous I/O can be acquired by means like IOCP's GetQueuedCompletionStatus() function.

On the other hand, asynchronous I/O making a difference requires that the thread have actual actions that can be performed not depending on the finish of the I/O. Else asynchronous I/O might not introduce optimization.

## unsolved problem
In Server.c, I intended to use epoll's edge trigger mode. However, use edge trigger mode (in Server.c, change `event.events = EPOLLIN;` to `event.events = EPOLLIN | EPOLLET;`) will ocassionally cause the browser keeps pending. Since browser keeps pending, at least the connection is still alive. But the connection should have been closed by my server code. Weird. Anyway, just leave the problem aside for now.