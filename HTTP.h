#pragma once

#include <string>
#include <map>
#include <functional>

#ifdef _WIN32
#	include <SDKDDKVer.h>
#	include <Windows.h>
#else
#	error Good luck.
#endif

namespace HTTP
{

enum METHOD
{
	METHOD_GET,
	METHOD_PUT,
	METHOD_POST,
	METHOD_DELETE,
	METHOD_HEAD,
	METHOD_OPTIONS
};

enum PROTOCOL
{
	PROTOCOL_HTTP_1_0,
	PROTOCOL_HTTP_1_1
};

enum AUTH_MODE
{
	AUTH_NONE,
	AUTH_BASIC
};

enum RESPONSE_CODE
{
	// Informational
	RESPONSE_CONTINUE = 100,			// Continue (please send body)
	RESPONSE_SWITCHING_PROTOCOLS = 101,	// Switching Protocols
	RESPONSE_PROCESSING = 102,			// Processing WebDAV operations

	// Success codes
	RESPONSE_OK = 200,					// OK
	RESPONSE_CREATED = 201,				// Created
	RESPONSE_ACCEPTED = 202,			// Request Accepted
	RESPONSE_NON_AUTHORITATIVE = 203,	// Returning information from untrusted source
	RESPONSE_NO_CONTENT = 204,			// No content returned by the server
	RESPONSE_RESET = 205,				// Like 204, but client must reset the view
	RESPONSE_PARTIAL = 206,				// Partial content being returned

	// Redirection codes
	RESPONSE_MOVED = 301,				// Response should be "URI: url comment endl"
	RESPONSE_FOUND = 302,				// Treated the same as 301
	RESPONSE_METHOD = 303,				// Reponse should be "Method: method url endl body"
	RESPONSE_NOTMODIFIED = 304,			// Like anyone cares about this.

	// Client error
	RESPONSE_BADREQUEST = 400,			// The client sent a malformed request
	RESPONSE_UNAUTHORISED = 401,		// The client isn't authorised for this resource
	RESPONSE_PAYMENTREQUIRED = 402,		// The client must pay to access this resource
	RESPONSE_FORBIDDEN = 403,			// The client can't access this
	RESPONSE_NOTFOUND = 404,			// It doesn't exist.

	// Server error
	RESPONSE_NOTIMPL = 500,				// The function requested isn't implemented.
	RESPONSE_TOOBUSY = 501,				// The server is too busy.
	RESPONSE_GATEWAYTIMEOUT = 502		// The gateway timed out.
};

enum REQUEST_PARSE_RESULT
{
	REQUEST_PARSE_OK,
	REQUEST_PARSE_MALFORMED,			// Unknown parsing error
	REQUEST_PARSE_UNKNOWN_METHOD,		// Expected GET or POST
	REQUEST_PARSE_UNKNOWN_PROTOCOL,		// Expected HTTP/1.1
	REQUEST_PARSE_MALFORMED_AUTH,		// Authorization data was corrupt
	REQUEST_PARSE_MALFORMED_CONTENT,	// Content attached to the header was corrupt
};

enum RESPONSE_HEADER_RESULT
{
	RESPONSE_HEADER_OK,
	RESPONSE_HEADER_NEED_REDIRECT_URI,
	RESPONSE_HEADER_NEED_AUTH_MODE,
	RESPONSE_HEADER_NEED_AUTH_REALM,
	RESPONSE_HEADER_NEED_CONTENT_MIME
};

enum SAFE_URI_ENCODE
{
	SAFE_URI_ENCODE_RFC_3986,				// !*'();:@&=+$,/\?%#[]
	SAFE_URI_ENCODE_RFC_3986_PATH,			// !*'();:@&=+$,?%#[]  (leaves / and \)
	SAFE_URI_ENCODE_ALL_NON_ALPHANUMERIC	// All characters that aren't A-Z/a-z/0-9
};

//
// These convert some of the various enums above into 
// ASCII strings.
//
LPCSTR MethodToString(_In_ METHOD m);
LPCSTR ProtocolToString(_In_ PROTOCOL p);
LPCSTR AuthModeToString(_In_ AUTH_MODE a);
LPCSTR ResponseCodeToString(_In_ RESPONSE_CODE rc);


typedef std::string String;
typedef const std::string& StringRef;
typedef std::map<std::string, std::string> StringTable;
typedef const StringTable& StringTableRef;

//
// When we receive data from browsers, they come prefixed
// with a header. Pass your data to RequestHeader.Parse
// to decode it.
//
// Explanation of some of the fields:
//
// Method:      This is either GET or POST. If it is POST,
//              you should use pPostDataOffsetOut to decode
//              post data that begins after the header.
//
// AuthMode:    If this is 'None', you can ignore it. In 
//              all other cases, this means that User and 
//              Password will contain the user's inputted 
//              credentials. 
//              NOTE: This HTTP server only supports Basic.
//
// ResourceURI: The URL that the user requested.
//              e.g. /my/path/my_page.txt?param1=foo&param2=bar
//              You can use the URI class to parse this.
// 
// Header:      This contains extra fields provided from 
//              the browser, indexable by name. A common
//              one is "User-Agent".
//
class RequestHeader
{
public:

	RequestHeader();
	~RequestHeader();

	METHOD Method() const;
	PROTOCOL Protocol() const;

	// This is 'None' if no credentials were supplied.
	AUTH_MODE AuthMode() const;
	StringRef AuthUser() const;
	StringRef AuthPassword() const;

	StringRef ResourceURI() const;
	
	// You can access other header information here.
	// e.g. Header()["User-Agent"]
	StringTableRef Header() const;

	//
	// Parses a header stream received from a browser.
	//
	// If there's any post data, it begins 'PostDataOffset'
	// bytes into the pRequestData stream.
	//
	REQUEST_PARSE_RESULT Parse(
		_In_ const char* pRequestData,
		_Out_opt_ SIZE_T* pPostDataOffsetOut);

private:

	struct REQUEST_DATA* m_pData;
};

//
// This is used to build a stream for sending back data
// to the browser.
//
class ResponseHeaderBuilder
{
public:

	ResponseHeaderBuilder();

	ResponseHeaderBuilder& 
	AddKey(
		_In_z_ StringRef Key,
		_In_z_ StringRef Value);

	RESPONSE_HEADER_RESULT 
	Build(
		_Out_ String& Output) const;

	//
	// Tools for constructing default web responses
	//
	RESPONSE_HEADER_RESULT 
	AddBinaryHeaders(
		_In_ SIZE_T ContentLength,
		_In_z_ LPCSTR MimeType);

	RESPONSE_HEADER_RESULT 
	AddTextHeaders(
		_In_ SIZE_T ContentLength,
		_In_z_ LPCSTR MimeType,
		_In_z_ LPCSTR Encoding);

	PROTOCOL Protocol;		// The HTTP protocol to use
	RESPONSE_CODE Code;		// The response code
	METHOD Method;			// For RESPONSE_METHOD only.
	String RedirectURI;		// Must specify redirect code to use this
	AUTH_MODE AuthMode;		// Set to non-None to request credentials
	String AuthRealm;		// Description of the authorization realm

private:

	StringTable m_ExtraLines;
};

//
// Utility to generate timestamps
//
String TimeStampString();

//
// Utility function for splitting HTTP-style parameter
// lists into key-value pairs.
//
// Example Input: sourceid=chrome&ie=UTF-8&q=foo+%26+bar
// Example Output:
//      Key         Value
//      ----------  ----------
//      ie          UTF-8
//      q           foo+%26+bar
//      sourceid    chrome
void 
ParseParameterList(
	_In_z_ LPCSTR ParameterString, 
	_Out_ StringTable& Output);

//
// Builds a parameter string from a StringTable. Make sure
// that the entries in the string table are URI safe.
//
String 
BuildParameterList(
	_In_ StringTableRef Parameters);

//
// Utility function for decoding URI-safe strings
//
// Example Input: foo%20%26%20bar
// Example Output: foo & bar
//
String 
DecodeURISafeString(
	_In_reads_(Count) LPCSTR URIString, 
	_In_ SIZE_T Count);

String 
DecodeURISafeString(
	_In_z_ LPCSTR URIString);

//
// Encode a string into a URI-safe string.
// Use SAFE_URI_ENCODE_RFC_3986 for most data.
// Use SAFE_URI_ENCODE_RFC_3986_PATH for URI paths.
// Use SAFE_URI_ENCODE_ALL_NON_ALPHANUMERIC for ALL symbols to be encoded.
//
String 
EncodeURISafeString(
	_In_reads_(Count) LPCSTR UnsafeString, 
	_In_ SIZE_T Count,
	_In_ SAFE_URI_ENCODE EncodeType);

String 
EncodeURISafeString(
	_In_z_ LPCSTR UnsafeString,
	_In_ SAFE_URI_ENCODE EncodeType);

//
// This decodes a URI. This will perform all encoding
// and parsing processes.
//
// For example, calling Parse on a URL such as:
//      /resource/path%20with%20spaces.ext?param=lolcats%20%26%20cake#anchor
// Will output:
//		Resource = "/resource/path with spaces.ext"
//      Anchor = "anchor"
//      Parameters = { "param", "lolcats & caek" }
//
// Likewise, calling ToString() with those members will
// construct the example string listed above.
//
class URI
{
public:

	String Resource;
	String Anchor;
	StringTable Parameters;

	//
	// This takes the resource and parameters and constructs
	// a string like this:
	//      /some/resource/path.ext?param1=foo&param2=bar#anchor
	//
	String ToString(
		_In_ bool MakeURISafe = true
		) const;

	// 
	// This takes a string like:
	//      /some/resource/path.ext?param1=foo&param2=bar#anchor
	// And splits the path into 'Resource' and the 
	// GET parameters into 'Parameters'.
	//
	// Existing Parameters will be erased.
	//
	void 
	ParseURIString(
		_In_z_ LPCSTR URIString);
};

// 
// Base64 decoding/encoding
// 
String
Base64Encode(
	_In_reads_(DataSize) LPCVOID pData,
	_In_ SIZE_T DataSize);

String 
Base64Encode(
	_In_ StringRef In);

String 
Base64Decode(
	_In_reads_(DataSize) LPCVOID pData,
	_In_ SIZE_T DataSize);

String 
Base64Decode(
	_In_ StringRef In);

//
// WebSockets API
//

enum WS_RESPONSE_RESULT
{
	WS_RESPONSE_OK,
	WS_RESPONSE_MISSING_KEY
};

// Returns true if the header looks like a Websocket request
bool IsWebsocketRequest(
	_In_ const RequestHeader& req);

// This takes in a string and generates a 160-bit SHA1 hash, returned through
// 5 UINTs.
typedef std::function<void (LPCSTR, LPUINT)> HashFunc;

WS_RESPONSE_RESULT
BuildWebsocketRequestResponse(
	_In_ const HTTP::RequestHeader& request,
	_In_ HashFunc HashFunction,
	_Out_ ResponseHeaderBuilder* responseBuilder);

enum WS_FRAME_RESULT
{
	WS_FRAME_OK,
	WS_FRAME_ERROR,

	// If you get this, it means you specified/received an OpCode
	// without FinalPacket set.
	WS_FRAME_FRAGMENTED_OPCODE
};

enum WS_FRAME_OPCODE
{
	WS_FRAME_OPCODE_CONTINUATION		= 0x0,
	WS_FRAME_OPCODE_TEXT				= 0x1,
	WS_FRAME_OPCODE_BINARY				= 0x2,
	WS_FRAME_OPCODE_CONNECTION_CLOSE	= 0x8,
	WS_FRAME_OPCODE_PING				= 0x9,
	WS_FRAME_OPCODE_PONG				= 0xA
};

enum WS_CLOSE_REASON
{
	WS_CLOSE_NORMAL						= 1000,
	WS_CLOSE_GOING_AWAY					= 1001,
	WS_CLOSE_PROTOCOL_ERROR				= 1002,
	WS_CLOSE_NO_DATA_HANDLER			= 1003,
	// 1004 Reserved
	// 1005 Reserved
	// 1006 Reserved
	WS_CLOSE_INCONSISTENT_DATA			= 1007,
	WS_CLOSE_VOILATES_POLICY			= 1008,
	WS_CLOSE_MESSAGE_TOO_LARGE			= 1009,
	WS_CLOSE_REQUIRES_EXTENSION			= 1010,
	WS_CLOSE_CANNOT_FULFILL_REQUEST		= 1011
	// 1012-2999 Reserved
};

struct WS_FRAME_INFO
{
	BYTE FinalPacket					: 1;
	BYTE RSV1							: 1;
	BYTE RSV2							: 1;
	BYTE RSV3							: 1;
	BYTE Masked							: 1;
	WS_FRAME_OPCODE OpCode				: 7;

	DWORD MaskingKey;
	ULONGLONG PayloadLength;
};

struct WS_PACKED_FRAME_HEADER
{
	BYTE Length; 
	BYTE Data[15];
};

WS_FRAME_RESULT 
ParseWebsocketFrame(
	_In_reads_(DataLength) LPCVOID pData,
	_In_ ULONGLONG DataLength,
	_Out_ WS_FRAME_INFO* pFrameInfo,
	_Out_ LPCVOID* pFramePayload);

WS_FRAME_RESULT
SetWebsocketFrame(
	_In_ const WS_FRAME_INFO* pFrameInfo,
	_Out_ WS_PACKED_FRAME_HEADER* pFrameHeader);

// Utility function for constructing a control frame
WS_FRAME_RESULT
SetWebsocketControlFrame(
	_In_ WS_FRAME_OPCODE OpCode,
	_In_ ULONGLONG PayloadLength,
	_Out_ WS_PACKED_FRAME_HEADER* pFrameHeader);

// Utility function for constructing a close frame
WS_FRAME_RESULT
SetWebsocketCloseFrame(
	_In_ WS_CLOSE_REASON Reason,
	_In_ ULONGLONG PayloadLength,
	_Out_ WS_PACKED_FRAME_HEADER* pFrameHeader);

// This is synonymous with UnmaskWebsocketFrame, but 
// is aliased for clarity.
void 
MaskWebsocketPayload(
	_In_reads_(DataLength) LPCVOID pData,
	_In_ ULONGLONG DataLength,
	_In_ DWORD MaskingKey,
	_Out_writes_(DataLength) LPVOID pOutData);

// This is synonymous with MaskWebsocketFrame, but 
// is aliased for clarity.
void 
UnmaskWebsocketPayload(
	_In_reads_(DataLength) LPCVOID pData,
	_In_ ULONGLONG DataLength,
	_In_ DWORD MaskingKey,
	_Out_writes_(DataLength) LPVOID pOutData);

}
