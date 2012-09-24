HTTP
====

This is a library I wrote recently for interpreting HTML requests from browsers and constructing responses.

Features:

- Parsing of HTTP requests from a browser.
- Constructing HTTP responses for sending back to a browser.
- "Basic" authentication handling.
- URI parsing utilities.
- WebSocket support.

Compatibility:

- You'll need to implement Base64Decode yourself. There's a few good examples around.
- This revision doesn't handle the WebSocket handshake hashing - todo!
- This requires C++11.
- This was written and tested on Windows 8 and Visual Studio 11 Beta.
- Some munging may be required for Linux, but there's no code that can't be easily ported.

Disclaimer:

- I release this without any promises of functionality or warranty. 
- I'm happy to be contacted if you have problems with any of this code.
