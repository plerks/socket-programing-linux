An HTTP server. Able to serve static files and Implemented basic function that user can configure url with its corresponding response.

The function is limited and not easy to use, far less than formal framework. And code is not mature enough, I did't write adequate error processing code and not suitable for handling abnormal situation.

There are [aio](https://man7.org/linux/man-pages/man7/aio.7.html) functions in linux but I did't use them in the implementation. For the [windows](https://github.com/plerks/socket-programing-windows) code I used asynchrous I/O.

Default c lib does not have common data structures like String or Map. And I did't implement one myself. So I need to define buf length as an estimated upper bound or write some other compromised code.

The book's chapter24 demonstrated how to code a basic HTTP server. But it's quite basic, just accept, create new thread to read, resolve and write, without using any concurrent mode introduced in the book before. And the author mentioned that because for normal HTTP protocol, the server just close the connetion after responsing (short connection), no time for IOCP/epoll to make much effect, so using IOCP/epoll won't bring much improvement. Despite that, I used epoll to implement the server (originate from chapter17/echo_EPLTserv.c).

Use epoll to monitor socket file descriptors, accept client connection request, put client fds into a queue (with a mutex to guarantee thread safety) when there gets data to read. And mutiple worker threads retrieve client fd and handle the request.

## How to run
In linux, to let the server listen at port 80 needs sudo privilege so I changed to port to 8000. If want to serve at 80, you can change the port number in DemoApplication.c and compile to run.

1.Open **HTTP_server** folder with VSCode and open src/DemoApplication.c to run.

2.Or
```
HTTP_server> gcc src/DemoApplication.c lib/Handlers.c lib/Server.c -I include -o src/DemoApplication
HTTP_server> ./src/DemoApplication
```

Here I note down how to create static/shared(dynamic) libraries and statically/dynamically link it. For just running the project, there's no need to create a static/shared library.

3.Or
```
HTTP_server/lib> gcc -c ./Handlers.c ./Server.c -I ../include
HTTP_server/lib> ar -cr libmyserver.a Handlers.o Server.o
HTTP_server> gcc ./src/DemoApplication.c -I include -L lib -lmyserver -o src/DemoApplication
HTTP_server> ./src/DemoApplication
```

4.Or
```
HTTP_server/lib> gcc -shared -fpic ./Handlers.c ./Server.c -I ../include -o libmyserver.so

Put libmyserver.so in a dynamic-link library search path, for example, /usr/lib.

HTTP_server> gcc ./src/DemoApplication.c -I include -L lib -lmyserver -o src/DemoApplication
HTTP_server> ./src/DemoApplication
```

## How to see the effect

I have put some static files under resources and configured some urls' response.

* visit <http://localhost:8000/index.html> for static file index.html.
* visit <http://localhost:8000/getdemo> for self-configured GET response.
* POST <http://localhost:8000/postdemo> for self-configured POST response. (With Postman or something else, only support when the request indicates its request body length by `Content-Length`)
* other requests for errorpage

## How the HTTP request/response body end is detected?

First of all, the server and client can both easily know the end of headers by detecting the emptyLine (detecting "\r\n\r\n").

HTTP protocol's typical process: client send -> server response -> server close the connection. (not considering HTTP persistent connection)

So the client can know that the server has sent all the response content by detect the close of connection (read() returns 0). Or the response may also have `Content-Length` header (usually no as far as I see), the client can detect the end by it. For this my implementation, I have't written a `Content-Length` header in the response, and if in Server.c the `close(client_sock);` is commented, Chrome browser just keeps waiting if visit <http://localhost:8000/index.html>, thus proved.

And as for how server detects the end of request body. If it's a GET request, then it's supposed to have no body (thus done). And if it's a POST request, see [this](https://stackoverflow.com/questions/4824451/detect-end-of-http-request-body), the request headers should indicate the body length by `Content-Length`(for fixed length) or `Transfer-Encoding: Chunked`(for uncertain length).

## unsolved problem
In Server.c, I intended to use epoll's edge trigger mode. However, use edge trigger mode (in Server.c, change `event.events = EPOLLIN;` to `event.events = EPOLLIN | EPOLLET;`) will ocassionally cause the browser keeps pending. Since browser keeps pending, at least the connection is still alive. But the connection should have been closed by my server code. Weird. Anyway, just leave the problem aside for now.