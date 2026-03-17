#include "pch.h"
#include "Session.h"

Session::Session(uint64_t sessionId, SOCKET socket) : _sessionId(sessionId), _socket(socket)
{
	cout << "[Session] " << _sessionId << "번 클라이언트\n";
}

Session::~Session()
{
	cout << "[Session] " << _sessionId << "번 세션 소멸" << endl;
}

void Session::Disconnect()
{
	if (_socket != INVALID_SOCKET)
	{
		cout << "[Session] " << _sessionId << "번 손님 퇴장!" << endl;
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}
}

void Session::Recv()
{
	ZeroMemory(&_recvOverlapped, sizeof(_recvOverlapped));
	_recvOverlapped.type = IO_TYPE::READ;

	WSABUF wsaBuf;
	wsaBuf.buf = _recvBuffer;
	wsaBuf.len = sizeof(_recvBuffer);

	DWORD flags = 0;
	DWORD recvLen = 0;

	if (SOCKET_ERROR == WSARecv(_socket, &wsaBuf, 1, &recvLen, &flags, &_recvOverlapped.overlapped, nullptr))
	{
		int errCode = WSAGetLastError();
		if (errCode != WSA_IO_PENDING)
		{
			cout << "Recv 예약 실패! ErrorCode: " << errCode << endl;
			Disconnect();
		}
	}

}

void Session::Send(char* sendBuffer, int32_t sendLen)
{
	ZeroMemory(&_sendOverlapped, sizeof(_sendOverlapped));
	_sendOverlapped.type = IO_TYPE::WRITE;

	WSABUF wsaBuf;
	wsaBuf.buf = sendBuffer;
	wsaBuf.len = sendLen;

	DWORD sendBytes = 0;

	if (SOCKET_ERROR == WSASend(_socket, &wsaBuf, 1, &sendBytes, 0, &_sendOverlapped.overlapped, nullptr))
	{
		int errCode = WSAGetLastError();
		if (errCode != WSA_IO_PENDING)
		{
			cout << "Send 예약 실패! ErrorCode: " << errCode << endl;
			Disconnect();
		}
	}
}
