#include "pch.h"
#include "NetworkManager.h"
#include "Session.h"

NetworkManager::NetworkManager()
{
}

NetworkManager::~NetworkManager()
{
	WSACleanup();
}

bool NetworkManager::StartServer(uint16_t port)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "[NetworkManager] WSAStartup 실패\n";
		return false;
	}

	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_iocpHandle == NULL)
	{
		cout << "[NetworkManager] IOCP 수신함 생성 실패\n";
		return false;
	}

	_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (_listenSocket == INVALID_SOCKET) return false;

	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);

	if (::bind(_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) return false;
	if (::listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR) return false;

	cout << "[NetworkManager] 서버 오픈 완료! 포트: " << port << " / 클라이언트 대기 중..." << endl;

	while (true)
	{
		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(clientAddr);

		SOCKET clientSocket = ::accept(_listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET) continue;

		Session* newSession = new Session(_sessionIDMaker++, clientSocket);
		_sessions.push_back(newSession);

		CreateIoCompletionPort((HANDLE)clientSocket, _iocpHandle, (ULONG_PTR)newSession, 0);

		newSession->Recv();
	}

	return true;
}
