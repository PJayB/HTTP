#include "stdafx.h"
#include "HTTP.h"

#include <sstream>
#include <time.h>

namespace HTTP
{

#define HTTP_LINE_ENDING	"\r\n";

ResponseHeaderBuilder::ResponseHeaderBuilder(const RESPONSE_HEADER_DESC* pDesc)
	: m_Protocol(pDesc->Protocol)
	, m_Code(pDesc->Code)
	, m_Method(pDesc->Method)
	, m_AuthMode(pDesc->AuthMode)
	, m_AuthRealm(pDesc->AuthRealm ? pDesc->AuthRealm : "")
	, m_RedirectURI(pDesc->RedirectURI ? pDesc->RedirectURI : "")
{
}

ResponseHeaderBuilder& ResponseHeaderBuilder::AddKey(
	StringRef A,
	StringRef B)
{
	if (A.size())
	{
		m_ExtraLines[A] = B;
	}

	return *this;
}

RESPONSE_HEADER_RESULT 
ResponseHeaderBuilder::Build(
	String& OutResponse) const
{
	std::stringstream response; 
	
	response << ProtocolToString(m_Protocol) << " ";

	//
	// Some codes need special cases
	//
	if (m_Code == RESPONSE_MOVED ||
		m_Code == RESPONSE_FOUND)
	{
		if (!m_RedirectURI.size())
		{
			return RESPONSE_HEADER_NEED_REDIRECT_URI;
		}

		response 
			<< "URI: " 
			<< m_RedirectURI
			<< HTTP_LINE_ENDING;
	}
	else if (m_Code == RESPONSE_METHOD)
	{
		if (!m_RedirectURI.size())
		{
			return RESPONSE_HEADER_NEED_REDIRECT_URI;
		}

		response 
			<< "Method: " 
			<< MethodToString(m_Method) 
			<< " "
			<< m_RedirectURI 
			<< HTTP_LINE_ENDING;
	}
	else
	{
		response << m_Code << " " << ResponseCodeToString(m_Code) << HTTP_LINE_ENDING;
	}

	//
	// If we require authentication, say as much:
	//
	if (m_Code == RESPONSE_UNAUTHORISED)
	{
		if (m_AuthMode == AUTH_NONE)
		{
			return RESPONSE_HEADER_NEED_AUTH_MODE;
		}

		if (!m_AuthRealm.size())
		{
			return RESPONSE_HEADER_NEED_AUTH_REALM;
		}

		response 
			<< "WWW-Authenticate: " 
			<< AuthModeToString(m_AuthMode) 
			<< "Realm=\""
			<< m_AuthRealm 
			<< "\"" 
			<< HTTP_LINE_ENDING;
	}

	//
	// Add the keys
	//
	for (auto pair : m_ExtraLines)
	{
		response << pair.first << ":";
		if (pair.second.size())
			response << " " << pair.second;
		response << HTTP_LINE_ENDING;
	}

	// Terminating line break
	response << HTTP_LINE_ENDING;

	OutResponse = response.str();

	return RESPONSE_HEADER_OK;
}

String IntToStr(SIZE_T T)
{
	std::stringstream s;
	s << T;
	return s.str();
}

RESPONSE_HEADER_RESULT ResponseHeaderBuilder::AddBinaryHeaders(
	SIZE_T ContentLength,
	LPCSTR MimeType)
{
	if (!MimeType || !*MimeType)
	{
		return RESPONSE_HEADER_NEED_CONTENT_MIME;
	}

	AddKey("Content-Type", MimeType);
	AddKey("Content-Length", IntToStr(ContentLength));
	AddKey("Connection", "close");

	return RESPONSE_HEADER_OK;
}

RESPONSE_HEADER_RESULT ResponseHeaderBuilder::AddTextHeaders(
	SIZE_T ContentLength,
	LPCSTR MimeType,
	LPCSTR Encoding)
{
	if (!MimeType || !*MimeType ||
		!Encoding || !*Encoding)
	{
		return RESPONSE_HEADER_NEED_CONTENT_MIME;
	}

	String mimeAndEncoding = MimeType;

	mimeAndEncoding += "; ";
	mimeAndEncoding += Encoding;

	AddKey("Content-Type", mimeAndEncoding);
	AddKey("Content-Length", IntToStr(ContentLength));
	AddKey("Connection", "close");

	return RESPONSE_HEADER_OK;
}

String TimeStampString()
{
 	struct tm timeinfo;
	time_t rawtime;

	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);

	char time_str[256];
	asctime_s(time_str, sizeof(time_str), &timeinfo);

	// chop of \n
	size_t len = strlen(time_str);
	time_str[len-1] = 0;

	return time_str;
}

}
