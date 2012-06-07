#include "stdafx.h"
#include "HTTP.h"

#include <sstream>
#include <time.h>

namespace HTTP
{

ResponseHeader::ResponseHeader()
{
}

ResponseHeader::~ResponseHeader()
{
}

LPCSTR ResponseHeader::Content() const
{
	return m_Data.c_str();
}

SIZE_T ResponseHeader::ContentLength() const
{
	return m_Data.size();
}

RESPONSE_HEADER_RESULT ResponseHeader::BuildHeader(
	const RESPONSE_HEADER_DESC* pDesc,
	LPCSTR ContentMimeType)
{
	std::stringstream response;

	m_Data = "";

	//
	// Validate stuff we definitely require
	//
	if (!ContentMimeType || !*ContentMimeType)
	{
		return RESPONSE_HEADER_NEED_CONTENT_MIME;
	}

	response << ProtocolToString(pDesc->Protocol) << " ";

	//
	// Some codes need special cases
	//
	if (pDesc->Code == RESPONSE_MOVED ||
		pDesc->Code == RESPONSE_FOUND)
	{
		if (!pDesc->RedirectURI || !*pDesc->RedirectURI)
		{
			return RESPONSE_HEADER_NEED_REDIRECT_URI;
		}

		response 
			<< "URI: " 
			<< pDesc->RedirectURI
			<< std::endl;
	}
	else if (pDesc->Code == RESPONSE_METHOD)
	{
		if (!pDesc->RedirectURI || !*pDesc->RedirectURI)
		{
			return RESPONSE_HEADER_NEED_REDIRECT_URI;
		}

		response 
			<< "Method: " 
			<< MethodToString(pDesc->Method) 
			<< " "
			<< pDesc->RedirectURI 
			<< std::endl;
	}
	else
	{
		response << ResponseCodeToString(pDesc->Code) << std::endl;
	}

	//
	// If we require authentication, say as much:
	//
	if (pDesc->Code == RESPONSE_UNAUTHORISED)
	{
		if (pDesc->AuthMode == AUTH_NONE)
		{
			return RESPONSE_HEADER_NEED_AUTH_MODE;
		}

		if (!pDesc->AuthRealm || !*pDesc->AuthRealm)
		{
			return RESPONSE_HEADER_NEED_AUTH_REALM;
		}

		response 
			<< "WWW-Authenticate: " 
			<< AuthModeToString(pDesc->AuthMode) 
			<< "Realm=\""
			<< pDesc->AuthRealm 
			<< "\"" 
			<< std::endl;
	}

	//
	// Append the date and time
	//
	{
		struct tm timeinfo;
		time_t rawtime;

		time(&rawtime);
		localtime_s(&timeinfo, &rawtime);

		char time_str[256];
		asctime_s(time_str, sizeof(time_str), &timeinfo);

		response 
			<< "Date: " 
			<< time_str;
	}

	//
	// Server name
	//
	if (pDesc->ServerName)
	{
		response << "Server: " << pDesc->ServerName << std::endl;
	}

	response << "Connection: close" << std::endl;

	//
	// Encoding for the content
	//
	response << "Content-Type: " << ContentMimeType << std::endl;
	response << "Content-Length: " << pDesc->ContentLength << std::endl;


	//
	// Copy the header now.
	//
	response << std::endl;

	m_Data = response.str();

	return RESPONSE_HEADER_OK;
}

RESPONSE_HEADER_RESULT ResponseHeader::BuildBinaryHeader(
	const RESPONSE_HEADER_DESC* pDesc,
	LPCSTR MimeType)
{
	return BuildHeader(
		pDesc, 
		MimeType);
}

RESPONSE_HEADER_RESULT ResponseHeader::BuildTextHeader(
	const RESPONSE_HEADER_DESC* pDesc,
	LPCSTR MimeType,
	LPCSTR Encoding)
{
	std::string mimeAndEncoding = MimeType;

	mimeAndEncoding += "; ";
	mimeAndEncoding += Encoding;

	return BuildHeader(
		pDesc, 
		mimeAndEncoding.c_str());
}

}
