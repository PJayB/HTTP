#include "HTTP.h"

namespace HTTP
{
LPCSTR MethodToString(METHOD m)
{
	switch (m)
	{
	case METHOD_GET:
		return "GET";
	case METHOD_PUT:
		return "PUT";
	case METHOD_POST:
		return "POST";
	case METHOD_DELETE:
		return "DELETE";
	case METHOD_HEAD:
		return "HEAD";
	case METHOD_OPTIONS:
		return "OPTIONS";
	default: 
		return nullptr;
	}
}

LPCSTR ProtocolToString(PROTOCOL p)
{
	switch (p)
	{
	case PROTOCOL_HTTP_1_0:
		return "HTTP/1.0";
	case PROTOCOL_HTTP_1_1:
		return "HTTP/1.1";
	default: 
		return nullptr;
	}
}

LPCSTR AuthModeToString(AUTH_MODE a)
{
	switch (a)
	{
	case AUTH_NONE:
		return "None";
	case AUTH_BASIC:
		return "Basic";
	default: 
		return nullptr;
	}
}

LPCSTR ResponseCodeToString(RESPONSE_CODE r)
{
	switch ( r )
	{
	// Informational
	case RESPONSE_CONTINUE:
		return "Continue"; 
	case RESPONSE_SWITCHING_PROTOCOLS:
		return "Switching Protocols";
	case RESPONSE_PROCESSING:
		return "Processing";

	// Success codes
	case RESPONSE_OK:
		return "OK"; 
	case RESPONSE_CREATED:
		return "Created"; 
	case RESPONSE_ACCEPTED:
		return "Accepted"; 
	case RESPONSE_NON_AUTHORITATIVE:
		return "Non-Authoritative Information";
	case RESPONSE_NO_CONTENT:
		return "No Content";
	case RESPONSE_RESET:
		return "Reset Content";
	case RESPONSE_PARTIAL:
		return "Partial Content";

	// Redirection codes
	case RESPONSE_MOVED:
	case RESPONSE_FOUND:
		//return "URI: "; 

	case RESPONSE_METHOD:
		//return "Method: "; 

	case RESPONSE_NOTMODIFIED:
		return ""; 

	// Client error
	case RESPONSE_BADREQUEST:
		return "Bad Request"; 
	case RESPONSE_UNAUTHORISED:
		return "Unauthorized"; 
	case RESPONSE_PAYMENTREQUIRED:
		return "Payment Required"; 
	case RESPONSE_FORBIDDEN:
		return "Forbidden"; 
	case RESPONSE_NOTFOUND:
		return "Not Found"; 

	// Server error
	case RESPONSE_NOTIMPL:
		return "Not Implemented"; 
	case RESPONSE_TOOBUSY:
		return "Server Busy"; 
	case RESPONSE_GATEWAYTIMEOUT:
		return "Gateway Time-Out"; 

	default:
		return nullptr;
	}
}

}
