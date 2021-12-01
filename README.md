#21 school webserv
Simple http/1.1 server based on kqueue. Supported methods: GET, POST, PUT, DELETE.

Other features: keep-alive, chunked-encoding, basic cgi, mime types config, virtual servers, redirects, client body size limit, file upload, custom error pages, directory listing, index file can be specified, allowed methods can be specified. 

Limited testing was performed. Tested on Mac OS only
#Usage

Replace /FULL_PATH_TO_FILE in config.
cgi_pass should be executable. Delete if not used.

make && ./webserv path_to_config


#Subject requirements
You can use every macro and define like FD_SET,FD_CLR,FD_ISSET,FD_ZERO (understanding what they do and how they do it is very useful.)
        You must write an HTTP server in C++ 98.
* If you need more C functions, you can use them but always prefer C++.
* The C++ standard must be C++ 98. Your project must compile with it.
* No external library, no Boost, etc...
* Try to always use the most "C++" code possible (for example use <cstring> over <string.h>).
* Your server must be compatible with the web browser of your choice.
* We will consider that Nginx is HTTP 1.1 compliant and may be used to compare
headers and answer behaviors.
* In the subject and the scale we will mention poll but you can use equivalent like select, kqueue, epoll.
* It must be non-blocking and use only 1 poll (or equivalent) for all the IO between the client and the server (listens includes).
3
Webserv This is when you finally understand why a URL starts with HTTP
* poll (or equivalent) should check read and write at the same time.
* Your server should never block and the client should be bounce properly if necessary.
* You should never do a read operation or a write operation without going through poll (or equivalent).
* Checking the value of errno is strictly forbidden after a read or a write operation.
* A request to your server should never hang forever.
* You server should have default error pages if none are provided.
* Your program should not leak and should never crash, (even when out of memory if all the initialization is done)
* You can’t use fork for something else than CGI (like php or python etc...)
* You can’t execve another webserver...
* Your program should have a config file in argument or use a default path.
* You don’t need to use poll (or equivalent) before reading your config file.
* You should be able to serve a fully static website.
* Client should be able to upload files.
* Your HTTP response status codes must be accurate.
* You need at least GET, POST, and DELETE methods.
* Stress tests your server it must stay available at all cost.
* Your server can listen on multiple ports (See config file).

Config requirments
choose the port and host of each "server"
* setup the server_names or not
* The first server for a host:port will be the default for this host:port (meaning it will answer to all request that doesn’t belong to an other server)
* setup default error pages
* limit client body size
* setup routes with one or multiple of the following rules/configuration (routes wont be using regexp):
  * define a list of accepted HTTP Methods for the route
  * define an HTTP redirection.
  * define a directory or a file from where the file should be search (for example if url /kapouet is rooted to /tmp/www, url /kapouet/pouic/toto/pouet is /tmp/www/pouic/toto/pouet)
  * turn on or off directory listing
  * default file to answer if the request is a directory
  * execute CGI based on certain file extension (for example .php)
* You wonder what a CGI is ?
* Because you won’t call the CGI directly use the full path as PATH_INFO
* Just remembers that for chunked request, your server needs to unchun- ked it and the CGI will expect EOF as end of the body.
* Same things for the output of the CGI. if no content_length is returned from the CGI, EOF will mean the end of the returned data.
* Your program should call the cgi with the file requested as first argu- ment
* the cgi should be run in the correct directory for relativ path file access
* your server should work with one CGI (php-cgi, python...)
** make the route able to accept uploaded files and configure where it should be saved