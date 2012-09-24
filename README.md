HTTP
====

This is a library I wrote recently for interpreting HTML requests from browsers and constructing responses. If you use this, please credit me accordingly.

This is still a work in progress. Feedback is welcome.

Features:

- Parsing of HTTP requests from a browser.
- Constructing HTTP responses for sending back to a browser.
- "Basic" authentication handling.
- URI parsing utilities.
- WebSocket support.

Compatibility:

- The Base64 encoding and decoding was taken from http://www.adp-gmbh.ch/win/misc/webserver.html
- This code doesn't handle the WebSocket handshake hashing; you'll need to provide your own. See below.
- This requires C++11.
- This was written and tested on Windows 8 and Visual Studio 11 Beta.
- Some munging may be required for Linux, but there's no code that can't be easily ported.

Disclaimer:

- I release this without any promises of functionality or warranty. 
- I'm happy to be contacted if you have problems with any of this code.
- This code is in no way affiliated with or endorsed by Microsoft.

WebSocket Handshake Hashing:

Currently I'm using a third party hashing library that isn't mine to distribute, but I can recommend the smallsha1 code found at http://code.google.com/p/smallsha1/.

Integrate it with this library by using the following code to build your response to the Websocket handshake:

	HTTP::HashFunc hashFunc = [] (LPCSTR pStrIn, LPUINT pHashOut)
	{
		SHA1 hash;
		hash << pStrIn;
		hash.Result((unsigned*) pHashOut);
	};

	HTTP::BuildWebsocketRequestResponse(
		request,
		hashFunc,
		&responseHeader);
