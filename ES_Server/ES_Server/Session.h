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
	void SendAccountId(int32_t accountId) { _accountId = accountId; }

	SOCKET GetSocket() { return _socket; }
	uint64_t GetSessionId() { return _sessionId; }
	char* GetRecvBuffer() { return _recvBuffer; }
	int32_t GetAccountId() { return _accountId; }


private:
	SOCKET _socket = INVALID_SOCKET;
	uint64_t _sessionId = 0;

	int32_t _accountId = 0;

	OverlappedEx _recvOverlapped;
	char _recvBuffer[1024];

	OverlappedEx _sendOverlapped;
};

