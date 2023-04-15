Echo server. Concurrent model based on epoll. Run along with chapter10/echo_mpclient.c.

echo_EPLTserv.c uses epoll's default level trigger mode.

echo_EPETserv.c uses non-blocking socket and epoll's edge trigger mode.

Non-blocking socket is better than blocking socket, since blocking socket may cause long-time stuckness.

Under edge trigger mode, the os generates less epoll_event. But I have no idea if there would be big difference in efficiency between level trigger and edge trigger in normal situation. For extreme situation, for example, the program can't read the data in time, edge trigger may have better efficiency.