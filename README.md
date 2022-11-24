#  webserv
Simple http/1.1 server based on kqueue. Supported methods: GET, POST, PUT, DELETE.

Other features: keep-alive, chunked-encoding, basic cgi, mime types config, virtual servers, redirects, client body size limit, file upload, custom error pages, directory listing, index file can be specified, allowed methods can be specified. 

Limited testing was performed. Tested on Mac OS only
## Usage

Replace "/FULL_PATH_TO_FILE" in webserv_config_example.
cgi_pass should be executable. Delete if not used.

```
make && ./webserv path_to_config
```
