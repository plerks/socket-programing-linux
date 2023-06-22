Echo server. Concurrent model based on select() function. Run along with chapter10/echo_mpclient.c.

However, [select() linux man page](https://man7.org/linux/man-pages/man2/select.2.html) warns that：
```
WARNING: select() can monitor only file descriptors 
numbers that are less than FD_SETSIZE (1024)—an 
unreasonably low limit for many modern 
applications—and this limitation will not change. All 
modern applications should instead use poll(2) or 
epoll(7), which do not suffer this limitation.
```