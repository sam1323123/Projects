
proxy_server.c is an implementation of a proxy server. 
The program takes in a port number and opens a listener for connection requests to clients.

The server handles various async and connection errors using POSIX signals.

A semaphore synchronized read/write data_cache is also added to store web data for more efficient service of content.  

