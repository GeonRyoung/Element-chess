#pragma once
#include "RecvBuffer.h"
#include "GameSession.h"

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
	void OnSendCompleted();

	void SendAccountId(int32_t accountId) { _accountId = accountId; }
	SOCKET GetSocket() { return _socket; }
	uint64_t GetSessionId() { return _sessionId; }
	RecvBuffer& GetRecvBuffer() { return _recvBuffer; }
	int32_t GetAccountId() { return _accountId; }

	void SetGameSession(unique_ptr<GameSession> gameSession) { _gameSession = move(gameSession); }
	GameSession* GetGameSession() { return _gameSession.get(); }
private:

	void RegisterSend();

	SOCKET _socket = INVALID_SOCKET;
	uint64_t _sessionId = 0;

	int32_t _accountId = 0;

	OverlappedEx _recvOverlapped;

	class RecvBuffer _recvBuffer{ 1024 };

	OverlappedEx _sendOverlapped;

	/*================
		Send Queue
	================*/

	std::mutex _sendLock;
	std::queue<std::vector<char>> _sendQueue;
	std::atomic<bool> _isSending = false;
	std::vector<char> _currentSendBuffer;
	unique_ptr<GameSession> _gameSession = nullptr;
};

