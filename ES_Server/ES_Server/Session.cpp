#include "pch.h"
#include "Session.h"
#include "RecvBuffer.h"
#include "GameSession.h"

Session::Session(uint64_t sessionId, SOCKET socket) : _sessionId(sessionId), _socket(socket)
{
	GLOG(Session, "%llu번 클라이언트", _sessionId);
}

Session::~Session()
{
	GLOG(Session, "%llu번 세션 및 게임 데이터 소멸", _sessionId);
}

void Session::Disconnect()
{
	if (_socket != INVALID_SOCKET)
	{
		GLOG(Session, "%llu번 클라이언트 퇴장!", _sessionId);
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}
}

void Session::Recv()
{
	ZeroMemory(&_recvOverlapped, sizeof(_recvOverlapped));
	_recvOverlapped.type = IO_TYPE::READ;

	WSABUF wsaBuf;
	wsaBuf.buf = _recvBuffer.WritePos();
	wsaBuf.len = _recvBuffer.FreeSize();

	DWORD flags = 0;
	DWORD recvLen = 0;

	if (SOCKET_ERROR == WSARecv(_socket, &wsaBuf, 1, &recvLen, &flags, &_recvOverlapped.overlapped, nullptr))
	{
		int errCode = WSAGetLastError();
		if (errCode != WSA_IO_PENDING)
		{
			GLOG_ERROR(Session, "Recv 예약 실패! ErrorCode: %d", errCode);
			Disconnect();
		}
	}

}

void Session::Send(char* sendBuffer, int32_t sendLen)
{
	std::vector<char> sendData(sendBuffer, sendBuffer + sendLen);

	{
		std::lock_guard<std::mutex> lock(_sendLock);
		_sendQueue.push(sendData);
	}

	if (_isSending.exchange(true) == false)
	{
		RegisterSend();
	}
}

void Session::RegisterSend()
{
	{
		std::lock_guard<std::mutex> lock(_sendLock);
		_currentSendBuffer = _sendQueue.front();
		_sendQueue.pop();
	}

	ZeroMemory(&_sendOverlapped, sizeof(_sendOverlapped));
	_sendOverlapped.type = IO_TYPE::WRITE;

	WSABUF wsaBuf;
	wsaBuf.buf = _currentSendBuffer.data();
	wsaBuf.len = (ULONG)_currentSendBuffer.size();

	DWORD sendBytes = 0;

	if (SOCKET_ERROR == WSASend(_socket, &wsaBuf, 1, &sendBytes, 0, &_sendOverlapped.overlapped, nullptr))
	{
		int errCode = WSAGetLastError();
		if (errCode != WSA_IO_PENDING)
		{
			GLOG_ERROR(Session, "Send 예약 실패! ErrorCode: %d", errCode);
			_isSending = false; 
			Disconnect();
		}
	}
}

void Session::OnSendCompleted()
{
	{
		std::lock_guard<std::mutex> lock(_sendLock);
		if (_sendQueue.empty())
		{
			_isSending = false;
			return;
		}
	}
	
	RegisterSend();
}
