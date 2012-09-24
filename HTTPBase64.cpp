#include "stdafx.h"
#include "HTTP.h"

namespace HTTP
{

/*
	WARNING:
	Shameless, shameless rip from http://www.adp-gmbh.ch/win/misc/webserver.html
*/

/*
	The character array
*/
const String kBase64Array = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
	Is the character in base64?
*/
bool __IsBase64( BYTE b )
{
	return ( 
		(b >= '0' && b <= '9') || 
		(b >= 'a' && b <= 'z') || 
		(b >= 'A' && b <= 'Z') ||
		b == '+' || 
		b == '/' );
}

/*
	Encode
*/
String Base64Encode(
	LPCVOID vpData, 
	SIZE_T Size )
{
	String ret;

	const BYTE* pData = (const BYTE *) vpData;
	int i = 0;
	int j = 0;
	BYTE char_array_3[3];
	BYTE char_array_4[4];

	while (Size--)
	{
		char_array_3[i++] = *(pData++);
		if (i == 3)
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; i < 4; i++)
				ret += kBase64Array[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; j < i + 1; j++)
			ret += kBase64Array[char_array_4[j]];

		while(i++ < 3)
			ret += '=';
	}

	return ret;
}
String Base64Encode( StringRef pData )
{
	return Base64Encode( pData.c_str(), pData.size() );
}

/*
	Decode
*/
String Base64Decode( 
	LPCVOID pData, 
	SIZE_T Size )
{
	String ret;

	String encoded_string( (const char *)pData );
	int in_len = (int)encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	BYTE char_array_4[4], char_array_3[3];

	while (in_len-- && ( encoded_string[in_] != '=') && __IsBase64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i ==4)
		{
			for (i = 0; i <4; i++)
				char_array_4[i] = (BYTE)kBase64Array.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = (BYTE)kBase64Array.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
			ret += char_array_3[j];
	}

	return ret;
}
String Base64Decode( StringRef pData )
{
	return Base64Decode( pData.c_str(), pData.size() );
}

}