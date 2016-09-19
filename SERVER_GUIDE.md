## QUIC server

###### QUIC server options

```bash
% ./quic_thread_server --help
Usage: quic_thread_server [options]
options:
-h, --help                     show this help message and exit
--quic_port=<port>             specify the quic port number to listen on
--proxy_timeout=<second>       specify timeout duration to avoid a proxy
                               response
--worker_count=<count>         specify the worker thread count
--dispatch_continuity=<count>  specify the dispatch continuity count
                               range [1, 32], default is 16
--send_buffer_size=<size>      specify the send buffer size
                               default size was 1452 * 30
--recv_buffer_size=<size>      specify the recv buffer size
                               default size was 256kb
--daemon                       daemonize a process
--stop                         stop a quic damon process
--proxy_pass=<url>             reverse proxy url
                               example: http://example.com:8080
--config=<config_file_path>    specify the quic server config file path
--keyfile=<key_file_path>      specify the ssl key file path
--certfile=<cert_file_path>    specify the ssl certificate file path
--bind_address=<ip>            specify the udp socket bind ip address
--log_dir=<log_dir>            specift the logging directory
--logging                      turn on files base logging
                               (default was turned off)
```

## QUIC Discovery

To use QUIC server, you must understand [QUIC Discovery](https://docs.google.com/document/d/1i4m7DbrWGgXafHxwl8SwIusY2ELUe8WX258xt2LFxPM/edit). 

Stellite needs to perform APN (application protocol negotiation) to be able to use the QUIC protocol and QUIC Discovery plays a part here. QUIC Discovery checks APN information from an HTTP header and informs the server that the QUIC protocol is available.

To use QUIC Discovery, you need to specify the "Alternative Service" in the HTTP request header as follows:

```bash
Alt-Svc: quic="[<hostname>|]:<port>"; p="1"; ma=<seconds>
```

* "hostname" is QUIC server's host. If left blank, the original HTTP/TCP server's host is used.
* "port" is QUIC server's UDP port to be accessed by QUIC Discovery.
* "ma" (Max-Age) is the number of seconds these Alt-Svc header values are cached and thus usable.

Note that only the default values (p="1" and ma="604800", where "604800" = 7 days) are permitted in the current version of 2015-09-08. You cannot modify probability and ma parameters in this version of the client.

For detailed information, please refer to the [QUIC Discovery docs](https://docs.google.com/document/d/1i4m7DbrWGgXafHxwl8SwIusY2ELUe8WX258xt2LFxPM/edit?pref=2&pli=1) from Google.
