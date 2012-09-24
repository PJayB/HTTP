#include "stdafx.h"
#include "HTTP.h"

#include <stdio.h>
#include <string>
#include <map>

namespace HTTP
{

/*
	REQUEST INTERNALS
*/
struct REQUEST_DATA
{
	REQUEST_DATA()
		: Method(METHOD_GET)
		, Protocol(PROTOCOL_HTTP_1_1)
		, AuthMode(AUTH_NONE)
	{
	}

	METHOD Method;
	PROTOCOL Protocol;
	AUTH_MODE AuthMode;
	String ResourceURI;
	String AuthUser;
	String AuthPassword;
	StringTable Header;
};

/*
	PARSING
*/

bool Parse(const char*& cursor, const char* compare)
{
	int compLen = strlen(compare);

	if (strncmp(cursor, compare, compLen) == 0)
	{
		cursor += compLen;
		return true;
	}

	return false;
}

void EatSpaces(const char*& cursor)
{
	while (
		*cursor &&
		*cursor != '\r' &&
		*cursor != '\n' && 
		isspace(*cursor)) 
	{ 
		cursor++; 
	}
}

const char* FindNextWhiteSpace(const char* cursor)
{
	while (*cursor && !isspace(*cursor)) 
	{ 
		cursor++; 
	}

	return cursor;
}

bool ExpectNewLine(const char*& cursor)
{
	if (cursor[0] == '\n')
	{
		cursor++;
		return true;
	}
	if (cursor[0] == '\r' && cursor[1] == '\n')
	{
		cursor += 2;
		return true;
	}
	return false;
}

void CopyIntoStdString(const char* begin, const char* end, String& out)
{
	size_t length = (size_t)(end - begin);

	out.resize(length);
	memcpy((char *)out.c_str(), begin, length);
}

String GetWord(const char*& cursor)
{
	ASSERT(!isspace(*cursor));

	const char* begin = cursor;

	cursor = FindNextWhiteSpace(cursor);

	String out;
	CopyIntoStdString(begin, cursor, out);

	return out;
}

/*
	FIRST LINE
*/

REQUEST_PARSE_RESULT
ParseRequestHeader(
	const char*& cursor,
	REQUEST_DATA* out)
{
	// 
	// The first token is either GET or POST. Check for this.
	// 
	INT methodIndex = 0;
	LPCSTR methodName = nullptr;
	while ((methodName = MethodToString((METHOD)methodIndex)) != nullptr)
	{
		if (Parse(cursor, methodName))
		{
			out->Method = (METHOD)methodIndex;
			break;
		}

		methodIndex++;
	}
	
	if (methodName == nullptr)
	{
		return REQUEST_PARSE_UNKNOWN_METHOD;
	}

	EatSpaces(cursor);

	// 
	// The next token is the requested resource.
	// 
	out->ResourceURI = GetWord(cursor);

	EatSpaces(cursor);

	// 
	// Finally, the protocol. If it isn't HTTP/1.1, reject.
	// 
	if (Parse(cursor, "HTTP/1.0"))
	{
		out->Protocol = PROTOCOL_HTTP_1_0;
	}
	else if (Parse(cursor, "HTTP/1.1"))
	{
		out->Protocol = PROTOCOL_HTTP_1_1;
	}
	else
	{
		return REQUEST_PARSE_UNKNOWN_PROTOCOL;
	}

	if (!ExpectNewLine(cursor)) 
	{
		return REQUEST_PARSE_MALFORMED;
	}

	return REQUEST_PARSE_OK;
}

/*
	PARSING KEY: VALUE PAIRS
*/
REQUEST_PARSE_RESULT 
ParseKeyValuePair(
	const char*& cursor,
	String& keyOut,
	String& valueOut)
{
	//
	// Consume letters until the first :
	//
	const char* key = cursor;
	while (
		*cursor && 
		*cursor != ':' && 
		*cursor != '\r' && 
		*cursor != '\n')
	{
		// Sanity check
		if (isspace(*cursor)) 
		{
			return REQUEST_PARSE_MALFORMED;
		}

		cursor++;
	}

	CopyIntoStdString(key, cursor, keyOut);

	// Skip the :
	cursor++;

	EatSpaces(cursor);

	// Extract the value
	const char* value = cursor;
	while (
		*cursor && 
		*cursor != '\r' && 
		*cursor != '\n')
	{
		cursor++;
	}

	CopyIntoStdString(value, cursor, valueOut);

	ExpectNewLine(cursor);
	//if (!ExpectNewLine(cursor)) 
	//{
	//	return REQUEST_PARSE_MALFORMED;
	//}

	return REQUEST_PARSE_OK;
}

/*
	AUTH
*/
bool DecodeAuth(
	const char* authString,
	REQUEST_DATA* out)
{
	const char* cursor = authString;

	// Ensure it's "Basic"
	if (!Parse(cursor, "Basic"))
	{
		return false;
	}

	EatSpaces(cursor);

	// Decode the message
	String credentials = Base64Decode(
		cursor,
		(SIZE_T) strlen(cursor));

	// Split it 
	String::size_type colonPos = credentials.find(':');
	out->AuthUser = credentials.substr(0, colonPos);
	out->AuthPassword = credentials.substr(colonPos + 1);
	out->AuthMode = AUTH_BASIC;

	return true;
}

/*
	REQUEST IMPLEMENTATION
*/
RequestHeader::RequestHeader()
	: m_pData(new REQUEST_DATA)
{
}

RequestHeader::~RequestHeader()
{
	delete m_pData;
}

METHOD RequestHeader::Method() const
{
	return m_pData->Method;
}

PROTOCOL RequestHeader::Protocol() const
{
	return m_pData->Protocol;
}

StringRef RequestHeader::ResourceURI() const
{
	return m_pData->ResourceURI;
}

AUTH_MODE RequestHeader::AuthMode() const
{
	return m_pData->AuthMode;
}

StringRef RequestHeader::AuthUser() const
{
	return m_pData->AuthUser;
}

StringRef RequestHeader::AuthPassword() const
{
	return m_pData->AuthPassword;
}

StringTableRef RequestHeader::Header() const
{
	return m_pData->Header;
}

/*
	PARSING
*/
REQUEST_PARSE_RESULT
RequestHeader::Parse(
	const char* pRequestData,
	SIZE_T* pPostDataOffsetOut)
{
	*m_pData = REQUEST_DATA();

	const char* cursor = (const char *) pRequestData;

	// 
	// Parse the first line for the method, protocol and URI
	// 
	REQUEST_PARSE_RESULT headerResult = ParseRequestHeader(
		cursor,
		m_pData);
	if (headerResult != REQUEST_PARSE_OK)
	{
		return headerResult;
	}

	// 
	// Now we're looking at a dictionary of possible keys and values.
	// 
	while (*cursor)
	{
		// 
		// An empty line means it's the end of the header block.
		// 
		if (ExpectNewLine(cursor))
		{
			break;
		}

		// 
		// Extract the key and value from the string.
		// 
		String key, value;
		REQUEST_PARSE_RESULT lineResult = ParseKeyValuePair(
			cursor, 
			key,
			value);
		if (lineResult != REQUEST_PARSE_OK)
		{
			return lineResult;
		}

		//
		// The authorization information shouldn't go into the Header array
		//
		if (key == "Authorization")
		{
			if (!DecodeAuth(value.c_str(), m_pData))
			{
				return REQUEST_PARSE_MALFORMED_AUTH;
			}

			continue;
		}

		m_pData->Header[key] = value;
	}

	if (pPostDataOffsetOut)
	{
		*pPostDataOffsetOut = (cursor - pRequestData);
	}

	return REQUEST_PARSE_OK;
}

/*
	GET/POST helpers
*/

void ParseParameterList(LPCSTR ParameterString, StringTable& out)
{
	while (ParameterString && *ParameterString)
	{
		// Consume stray apersands.
		while (*ParameterString && *ParameterString == '&')
		{
			ParameterString++;
		}

		LPCSTR Begin = ParameterString;

		// Find the next equals.
		while (
			*ParameterString && 
			*ParameterString != '=' &&
			*ParameterString != '&')
		{
			ParameterString++;
		}

		if (ParameterString == Begin)
		{
			continue;
		}

		String key, value;

		CopyIntoStdString(Begin, ParameterString, key);

		if (*ParameterString == '=')
		{
			Begin = ++ParameterString;

			while (
				*ParameterString &&
				*ParameterString != '&')
			{
				ParameterString++;
			}

			CopyIntoStdString(Begin, ParameterString, value);
		}

		out[key] = value;
	}
}

String BuildParameterList(StringTableRef Parameters)
{
	String out;

	bool needAnd = false;
	for (auto i = std::begin(Parameters); i != std::end(Parameters); ++i)
	{
		if (i->first.size() > 0)
		{
			if (needAnd)
			{
				out += "&";
			}

			out += i->first;

			if (i->second.size() > 0)
			{
				out += '=';
				out += i->second;
			}

			needAnd = true;
		}
	}

	return out;
}

bool IsHexDigit(BYTE c)
{
	return (c >= '0' && c <= '9') || 
		   (c >= 'a' && c <= 'f') ||
		   (c >= 'A' && c <= 'F');
}

BYTE HexValue(BYTE c)
{
	ASSERT(IsHexDigit(c));

	if (c >= '0' && c <= '9')
	{
		return c - '0';
	}
	else if (c >= 'a' && c <= 'z')
	{
		return c - 'a' + 10;
	}
	else if (c >= 'A' && c <= 'Z')
	{
		return c - 'A' + 10;
	}

	// Should never get here.
	ASSERT(0);
	return 0;
}

bool DecodeURISafeCharFromHex(BYTE Hi, BYTE Lo, BYTE* out)
{
	if (!Hi || !Lo ||
		!IsHexDigit(Hi) ||
		!IsHexDigit(Lo)) 
	{
		return false;
	}

	Hi = HexValue(Hi);
	Lo = HexValue(Lo);

	*out = Hi << 4 | Lo;

	return true;
}

String DecodeURISafeString(LPCSTR URIString, SIZE_T Count)
{
	String out;

	out.reserve(strlen(URIString));

	SIZE_T i = 0;
	while (URIString && i < Count)
	{
		if (URIString[i] == '%')
		{
			BYTE c = 0;
			if (DecodeURISafeCharFromHex(
				URIString[i + 1],
				URIString[i + 2], 
				&c) && c >= 0x20)
			{
				out += c;
				i += 3;
				continue;
			}
		}
		
		out += URIString[i++];
	}

	return out;
}

String DecodeURISafeString(LPCSTR URIString)
{
	return DecodeURISafeString(URIString, strlen(URIString));
}

bool IsAlphaNumeric(CHAR c)
{
	return (c >= 'a' && c <= 'z') ||
		   (c >= 'A' && c <= 'Z') ||
		   (c >= '0' && c <= '9');
}

BYTE ToHex(BYTE c)
{
	ASSERT(c <= 0xF);

	if (c < 10) { return c + '0'; }
	else { return c - 10 + 'A'; }
}

bool EncodeURISafeCharToHex(BYTE c, BYTE* Hi, BYTE* Lo)
{
	if (c < 0x20)
	{
		return false;
	}

	*Hi = ToHex((c & 0xF0) >> 4);
	*Lo = ToHex((c & 0x0F));

	return true;
}

bool CharIsEligibleForRFC3986(CHAR c)
{
	LPCSTR Criteria = " !*'\"();:@&=+$,?%#[]";

	while (*Criteria)
	{
		if (*Criteria++ == c)
		{
			return true;
		}
	}
	
	return false;
}

bool CharIsEligibleForEncode(CHAR c, SAFE_URI_ENCODE EncodeType)
{
	switch (EncodeType)
	{
	case SAFE_URI_ENCODE_RFC_3986:
		return	c == '\\' ||
				c == '/' ||
				CharIsEligibleForRFC3986(c);
	case SAFE_URI_ENCODE_RFC_3986_PATH:
		return CharIsEligibleForRFC3986(c);
	case SAFE_URI_ENCODE_ALL_NON_ALPHANUMERIC:
		return !IsAlphaNumeric(c);
	};

	ASSERT(0);
	return false;
}

String EncodeURISafeString(LPCSTR UnsafeString, SIZE_T Count, SAFE_URI_ENCODE EncodeType)
{
	String out;

	out.reserve(strlen(UnsafeString));

	SIZE_T i = 0;
	while (UnsafeString && i++ < Count)
	{
		if (CharIsEligibleForEncode(*UnsafeString, EncodeType))
		{
			BYTE Hi = 0;
			BYTE Lo = 0;
			if (EncodeURISafeCharToHex(
				*UnsafeString,
				&Hi, &Lo))
			{
				out += '%';
				out += Hi;
				out += Lo;
			}
		}
		else
		{
			out += *UnsafeString;
		}

		UnsafeString++;
	}

	return out;
}

String EncodeURISafeString(LPCSTR UnsafeString, SAFE_URI_ENCODE EncodeType)
{
	return EncodeURISafeString(UnsafeString, strlen(UnsafeString), EncodeType);
}

void URI::ParseURIString(LPCSTR URIString)
{
	Resource = "";
	Anchor = "";
	Parameters.clear();

	LPCSTR Path = URIString;
	while (*URIString && *URIString != '?' && *URIString != '#')
	{
		URIString++;
	}

	Resource = DecodeURISafeString(
		Path,
		URIString - Path);

	if (*URIString == '?')
	{
		String paramString;

		LPCSTR ParamsBegin = ++URIString;
		while (*URIString && *URIString != '#')
		{
			URIString++;
		}

		CopyIntoStdString(ParamsBegin, URIString, paramString);

		// This will generate a string table of the encoded parameters
		StringTable encodedParams;
		ParseParameterList(paramString.c_str(), encodedParams);

		// Decode these strings
		for (auto p = std::begin(encodedParams); 
			 p != std::end(encodedParams); ++p)
		{
			String decodedKey = DecodeURISafeString(p->first.c_str());
			String decodedValue = DecodeURISafeString(p->second.c_str());

			Parameters[decodedKey] = decodedValue;
		}
	}

	if (*URIString == '#')
	{
		while (*URIString == '#') { URIString++; }

		Anchor = DecodeURISafeString(URIString);
	}
}

String URI::ToString(bool MakeURISafe) const
{
	String out;

	out = MakeURISafe
		? EncodeURISafeString(Resource.c_str(), SAFE_URI_ENCODE_RFC_3986_PATH)
		: Resource;

	if (Parameters.size())
	{
		out += '?';

		bool needAnd = false;

		for (auto p = std::begin(Parameters); p != std::end(Parameters); ++p)
		{
			if (needAnd) 
			{
				out += '&';
			}

			needAnd = true;

			out += MakeURISafe 
				? EncodeURISafeString(p->first.c_str(), SAFE_URI_ENCODE_RFC_3986) 
				: p->first;
			if (p->second.size())
			{
				out += '=';
				out += MakeURISafe 
					? EncodeURISafeString(p->second.c_str(), SAFE_URI_ENCODE_RFC_3986) 
					: p->second;
			}
		}
	}

	if (Anchor.size())
	{
		out += '#';
		out += MakeURISafe
			? EncodeURISafeString(Anchor.c_str(), SAFE_URI_ENCODE_RFC_3986) 
			: Anchor;
	}

	return out;
}

}
