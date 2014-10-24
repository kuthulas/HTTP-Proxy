ECEN602 HW3 Programming Assignment 
-----------------------------------------------------------------

Team Number: 31
Member 1 Kumaran, Thulasiraman (UIN: 223003944)
Member 2 Manjunath, Jnanesh (UIN: 322005490)
---------------------------------------

Description:
--------------------
A HTTP 1.0 (RFC 1945) proxy server and a client are implemented using POSIX in C. A cache is designed based on doubly-linked list, and has a max of 10 entries & follows LRU replacement policy.

1. The proxy server parses requests from client & accordingly corresponds with the server before serving the client. The proxy can handle multiple simultaneous connections from different clients with different requests.
2. If the requested page is found in cache and is not expired, the proxy server sends a conditional GET to web server for checking if the page has been modified since the last accessed time.
3. If not modified the proxy sends the cached content to the client, else it downloads the file again from the server & forwards it to the client.
4. The cached pages are managed based on the expiry times and the modified times.

Unix command for starting proxy server:
------------------------------------------
./proxy PROXY_SERVER_IP PROXY_SERVER_PORT

Unix command for starting client:
----------------------------------------------
./client PROXY_SERVER_IP PROXY_SERVER_PORT URL