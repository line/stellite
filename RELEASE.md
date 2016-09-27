# Release Notes

## Stellite 0.1.0

First release announced on September 29, 2016

##### Summary

 * Stellite client library
   * Supports QUIC, SPDY, HTTP, HTTPS, HTTP2
   * Supports Content-Encoding filter (decoding)
   * Supports HTTP proxy
  
 * Stellite QUIC server
   * Supports reverse proxy
   * Supports multi-threading
   * Supports non-blocking file logging
   * Uses the SO_REUSEPORT socket option for UDP socket binding 
 
 * Build 
   * Outputs: Stellite client library, Stellite QUIC server
   * Uses Docker
   * Uses a Python build script to fetch the Chromium code and toolchains for target platforms
