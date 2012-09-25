#include "HTTP.h"
#include <assert.h>

namespace HTTP
{

BYTE AllBitsSet(BYTE byte, BYTE bit)
{
	return (byte & bit) == bit;
}

template<class T> T ByteSwap(T original)
{
	union __swap
	{
		BYTE b[sizeof(T)];
		T t;
	};

	__swap a;
	__swap b;
	
	a.t = original;

	for (size_t i = 0; i < sizeof(T); ++i)
		b.b[sizeof(T)-i-1] = a.b[i];

	return b.t;
}

bool IsWebsocketRequest(_In_ const RequestHeader& req)
{
	return 
		req.Header().find("Upgrade") != std::end(req.Header()) &&
		req.Header().find("Sec-WebSocket-Key") != std::end(req.Header());
}

WS_RESPONSE_RESULT
BuildWebsocketRequestResponse(
	_In_ const HTTP::RequestHeader& request,
	_In_ HashFunc HashFunction,
	_Out_ ResponseHeaderBuilder* responseHeader)
{
	String wsKey = request.Header().find("Sec-WebSocket-Key")->second;
	if (!wsKey.size())
		return WS_RESPONSE_MISSING_KEY;

	wsKey += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	/*
	SHA1 hash;
	hash << wsKey.c_str();
	hash.Result(wsKeyHash);
	*/

	UINT wsKeyHash[5] = {0, 0, 0, 0, 0};
	HashFunction(wsKey.c_str(), wsKeyHash);

	for (int i = 0; i < 5; ++i) 
		wsKeyHash[i] = ByteSwap(wsKeyHash[i]);

	wsKey = Base64Encode(wsKeyHash, sizeof(unsigned) * 5);

	responseHeader->Protocol = HTTP::PROTOCOL_HTTP_1_1;
	responseHeader->Code = HTTP::RESPONSE_SWITCHING_PROTOCOLS;

	responseHeader->AddKey("Upgrade", "websocket");
	responseHeader->AddKey("Connection", "Upgrade");
	responseHeader->AddKey("Sec-WebSocket-Accept", wsKey);
//	responseHeader->AddKey("Sec-WebSocket-Extensions", "");
	responseHeader->AddKey("Sec-WebSocket-Version", "17");

	return WS_RESPONSE_OK;
}

WS_FRAME_RESULT
ParseWebsocketFrame(
	_In_reads_(DataLength) LPCVOID pData,
	_In_ ULONGLONG DataLength,
	_Out_ WS_FRAME_INFO* pFrameInfo,
	_Out_ LPCVOID* pFramePayload)
{
	if (!pData || DataLength < 2 || !pFrameInfo || !pFramePayload)
		return WS_FRAME_ERROR;

	LPCBYTE pFrameHeader = (LPCBYTE) pData;

	ZeroMemory(pFrameInfo, sizeof(*pFrameInfo));

	pFrameInfo->FinalPacket = AllBitsSet(pFrameHeader[0], 0x80);
	pFrameInfo->RSV1 = AllBitsSet(pFrameHeader[0], 0x40);
	pFrameInfo->RSV2 = AllBitsSet(pFrameHeader[0], 0x20);
	pFrameInfo->RSV3 = AllBitsSet(pFrameHeader[0], 0x10);
	pFrameInfo->OpCode = (WS_FRAME_OPCODE) (pFrameHeader[0] & 0x0F);

	pFrameInfo->Masked = AllBitsSet(pFrameHeader[1], 0x80);
	
	LPCBYTE pFrameData = pFrameHeader + 2;

	BYTE payloadLengthBits = pFrameHeader[1] & 0x7F;
	if (payloadLengthBits == 127)
	{
		pFrameInfo->PayloadLength = ByteSwap((* (ULONGLONG*) pFrameData) & 0x7FFFFFFFFFFFFFFF);
		pFrameData += sizeof(ULONGLONG);
	}
	else if (payloadLengthBits == 126)
	{
		pFrameInfo->PayloadLength = ByteSwap(* (USHORT*) pFrameData);
		pFrameData += sizeof(USHORT);
	}
	else
	{
		pFrameInfo->PayloadLength = payloadLengthBits;
	}

	if (pFrameInfo->Masked)
	{
		pFrameInfo->MaskingKey = * (DWORD*) pFrameData;
		pFrameData += sizeof(DWORD);
	}

	*pFramePayload = pFrameData;

	if ((pFrameInfo->OpCode & 0x80) && !pFrameInfo->FinalPacket)
		return WS_FRAME_FRAGMENTED_OPCODE;

	return WS_FRAME_OK;
}

WS_FRAME_RESULT
SetWebsocketFrame(
	_In_ const WS_FRAME_INFO* pFrameInfo,
	_Out_ WS_PACKED_FRAME_HEADER* pFrameHeader)
{
	if (!pFrameInfo || !pFrameHeader)
		return WS_FRAME_ERROR;

	if ((pFrameInfo->OpCode & 0x80) && !pFrameInfo->FinalPacket)
		return WS_FRAME_FRAGMENTED_OPCODE;

	ZeroMemory(pFrameHeader, sizeof(WS_PACKED_FRAME_HEADER));

	LPBYTE pOut = pFrameHeader->Data;
	LPBYTE pExtraOut = pOut + 2;
	pFrameHeader->Length = 2;

	if (pFrameInfo->FinalPacket) pOut[0] |= 0x80;
	if (pFrameInfo->RSV1) pOut[0] |= 0x40;
	if (pFrameInfo->RSV2) pOut[0] |= 0x20;
	if (pFrameInfo->RSV3) pOut[0] |= 0x10;

	pOut[0] |= pFrameInfo->OpCode & 0xF;

	if (pFrameInfo->Masked) pOut[1] |= 0x80;

	if (pFrameInfo->PayloadLength > 0xFFFF)
	{
		pOut[1] |= 0x7F;
		pFrameHeader->Length += sizeof(ULONGLONG);
		(*(ULONGLONG*) pExtraOut) = pFrameInfo->PayloadLength;
		pExtraOut += sizeof(ULONGLONG);
	}
	else if (pFrameInfo->PayloadLength > 0x7E)
	{
		pOut[1] |= 0x7E;
		pFrameHeader->Length += sizeof(USHORT);
		(*(USHORT*) pExtraOut) = (USHORT) pFrameInfo->PayloadLength;
		pExtraOut += sizeof(USHORT);
	}
	else
	{
		pOut[1] |= (BYTE) pFrameInfo->PayloadLength & 0x7F;
	}
	
	if (pFrameInfo->Masked)
	{
		pFrameHeader->Length += sizeof(DWORD);
		(*(DWORD*) pExtraOut) = pFrameInfo->MaskingKey;
		pExtraOut += sizeof(DWORD);
	}

	assert(pFrameHeader->Length <= sizeof(pFrameHeader->Data));

	return WS_FRAME_OK;
}

WS_FRAME_RESULT
SetWebsocketControlFrame(
	_In_ WS_FRAME_OPCODE OpCode,
	_In_ ULONGLONG PayloadLength,
	_Out_ WS_PACKED_FRAME_HEADER* pFrameHeader)
{
	WS_FRAME_INFO frameInfo;
	ZeroMemory(&frameInfo, sizeof(frameInfo));

	frameInfo.FinalPacket = 1;
	frameInfo.OpCode = OpCode;
	frameInfo.PayloadLength = PayloadLength;

	return SetWebsocketFrame(
		&frameInfo,
		pFrameHeader);
}

WS_FRAME_RESULT
SetWebsocketCloseFrame(
	_In_ WS_CLOSE_REASON Reason,
	_In_ ULONGLONG PayloadLength,
	_Out_ WS_PACKED_FRAME_HEADER* pFrameHeader)
{
	WS_FRAME_INFO frameInfo;
	ZeroMemory(&frameInfo, sizeof(frameInfo));

	frameInfo.FinalPacket = 1;
	frameInfo.OpCode = WS_FRAME_OPCODE_CONNECTION_CLOSE;
	frameInfo.PayloadLength = PayloadLength + 2;

	WS_FRAME_RESULT r = SetWebsocketFrame(
		&frameInfo,
		pFrameHeader);

	if (r != WS_FRAME_OK)
		return r;

	assert(pFrameHeader->Length <= sizeof(pFrameHeader->Data) - 2);

	BYTE i = pFrameHeader->Length;
	pFrameHeader->Data[i++] = (BYTE) ((Reason & 0xFF00) >> 8);
	pFrameHeader->Data[i++] = (BYTE) (Reason & 0xFF);
	pFrameHeader->Length = i;

	return WS_FRAME_OK;
}

void 
MaskWebsocketPayload(
	_In_reads_(DataLength) LPCVOID pData,
	_In_ ULONGLONG DataLength,
	_In_ DWORD MaskingKey,
	_Out_writes_(DataLength) LPVOID pOutData)
{
	LPCBYTE pInDataBytes = (LPCBYTE) pData;
	LPCBYTE MaskBytes = (LPCBYTE) &MaskingKey;
	LPBYTE pOutDataBytes = (LPBYTE) pOutData;

	for (ULONGLONG i = 0; i < DataLength; ++i)
	{
		pOutDataBytes[i] = pInDataBytes[i] ^ MaskBytes[i % 4];
	}
}

void 
UnmaskWebsocketPayload(
	_In_reads_(DataLength) LPCVOID pData,
	_In_ ULONGLONG DataLength,
	_In_ DWORD MaskingKey,
	_Out_writes_(DataLength) LPVOID pOutData)
{
	MaskWebsocketPayload(pData, DataLength, MaskingKey, pOutData);
}

}
