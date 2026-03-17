#pragma once

enum class IO_TYPE
{
	READ,
	WRITE
};

struct OverlappedEx
{
	WSAOVERLAPPED overlapped;
	IO_TYPE type;
};

class Session
{
public:
	Session(uint64_t sessionId, SOCKET socekt);
	~Session();

	void Disconnect();

	void Recv();
	void Send(char* sendBuffer, int32_t sendLen);

	SOCKET GetSocket() { return _socket; }
	uint64_t GetSessionId() { return _sessionId; }

private:
	SOCKET _socket = INVALID_SOCKET;
	uint64_t _sessionId = 0;

	OverlappedEx _recvOverlapped;
	char _recvBuffer[1024];

	OverlappedEx _sendOverlapped;
};

