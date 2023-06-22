## What's this about

Here is my learning on socket programing. Based on a book called *[TCP/IP网络编程](https://www.ituring.com.cn/book/1284)* (ISBN 9787115358851).

The code is in c and not cross-platform. Here is code for linux. See the code for windows [here](https://github.com/plerks/socket-programing-windows).

The **bookcode** folder contains code originates from the book's example code (Not all example code is contained). And I may have adjusted the code style based on my own habit or added some adjustment.

The **HTTP_server** folder contains code implementing an HTTP server. The book's chapter24 demonstrated how to code a basic HTTP server. But it's quite basic, just accept, create new thread to read, resolve and write, without using any concurrent mode introduced in the book before. And the author mentioned that because for normal HTTP protocol, the server just close the connetion after responsing (short connection), no time for IOCP/epoll to make much effect, so using IOCP/epoll won't bring much improvement. Despite that, I used epoll to implement the server (originate from chapter17/echo_EPETserv.c).

I'm following under_score_case naming convention for the linux code.

`gcc --version`:
```
gcc (Ubuntu 11.3.0-1ubuntu1~22.04) 11.3.0
Copyright (C) 2021 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

## How to run

The **bookcode** and **HTTP_server**... folders have different VSCode running configuration. So I used VSCode's mutiple workspace function: a .code-workspace file has declared mutiple projects. As long as you open the **socket_programing.code-workspace** file with VSCode, the projects will be treated as individual workspace.

So the approaches to run are:

1.Open the **socket_programing.code-workspace** file with VSCode and run corresponding src file with VSCode. If you have opened the whole project folder(namely the parent folder of socket_programing.code-workspace), you need to open the socket_programing.code-workspace file and click the "Open Workspace" VSCode UI button at the bottom-right corner to reopen, else the VSCode configuration would be incorrect.

2.Or you can open **bookcode** and **HTTP_server**... folders respectively with VSCode and run.

3.Or you can compile and run with command line.

For more details, see the secondary README.md files.