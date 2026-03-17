#pragma once

class NetworkManager
{
public:
	NetworkManager();
	~NetworkManager();

	bool StartServer(uint16_t port);

	void WorkerThreadMain(HANDLE iocpHandle);

private:
	SOCKET _listenSocket = INVALID_SOCKET;
	HANDLE _iocpHandle = INVALID_HANDLE_VALUE;

	std::vector<class Session*> _sessions;
	uint64_t _sessionIDMaker = 1;

	std::vector<thread> _workerThreads;
};

