#include "stdafx.h"
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
	LPCSTR codesz = nullptr;

	switch ( r )
	{
	// Success codes
	case RESPONSE_OK:
	case RESPONSE_CREATED:
	case RESPONSE_ACCEPTED:
		codesz = "OK"; break;

	case RESPONSE_PARTIAL:
		codesz = "Partial Data"; break;
	case RESPONSE_NORESPONSE:
		codesz = "No Response"; break;

	// Redirection codes
	case RESPONSE_MOVED:
	case RESPONSE_FOUND:
		//codesz = "URI: "; 
		break;

	case RESPONSE_METHOD:
		//codesz = "Method: "; 
		break;

	case RESPONSE_NOTMODIFIED:
		codesz = ""; break;

	// Client error
	case RESPONSE_BADREQUEST:
		codesz = "Bad Request"; break;
	case RESPONSE_UNAUTHORISED:
		codesz = "Unauthorized"; break;
	case RESPONSE_PAYMENTREQUIRED:
		codesz = "Payment Required"; break;
	case RESPONSE_FORBIDDEN:
		codesz = "Forbidden"; break;
	case RESPONSE_NOTFOUND:
		codesz = "Not Found"; break;

	// Server error
	case RESPONSE_NOTIMPL:
		codesz = "Not Implemented"; break;
	case RESPONSE_TOOBUSY:
		codesz = "Server Busy"; break;
	case RESPONSE_GATEWAYTIMEOUT:
		codesz = "Gateway Time-Out"; break;

	default:
		break;
	}

	return codesz;
}

}
