#include "pch.h"
#include "NetworkManager.h"
#include "Session.h"
#include "DBManager.h"
#include "ClientPacketHandler.h"
#include "RecvBuffer.h"

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
		GLOG_ERROR(NetworkManager, "WSAStartup 실패");
		return false;
	}


	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_iocpHandle == NULL)
	{
		GLOG_ERROR(NetworkManager, "IOCP 수신함 생성 실패");
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

	GLOG(NetworkManager, "서버 오픈 완료! 포트: %d / 클라이언트 대기 중", port);

	uint32_t coreCount = std::thread::hardware_concurrency();
	for (uint32_t i = 0; i < coreCount; ++i)
	{
		_workerThreads.emplace_back([this]() { this->WorkerThreadMain(_iocpHandle); });
	}

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

void NetworkManager::WorkerThreadMain(HANDLE iocpHandle)
{
	while (true)
	{
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;
		WSAOVERLAPPED* overlapped = nullptr;

		bool ret = GetQueuedCompletionStatus(
			iocpHandle,
			&bytesTransferred,
			&completionKey,
			&overlapped,
			INFINITE
		);

		Session* session = (Session*)completionKey;

		if (ret == false || bytesTransferred == 0)
		{
			if (session != nullptr)
			{
				session->Disconnect();
			}
			continue;
		}

		OverlappedEx* overlappedEx = (OverlappedEx*)overlapped;

		if (overlappedEx->type == IO_TYPE::READ)
		{
			GLOG(Session, "Session %llu %lu 바이트의 데이터를 수신했습니다!", session->GetSessionId(), bytesTransferred);

			if (session->GetRecvBuffer().OnWrite(bytesTransferred) == false)
			{
				session->Disconnect();
				continue;
			}

			while (true)
			{
				int32_t dataSize = session->GetRecvBuffer().DataSize();
				if (dataSize < sizeof(PacketHeader)) break;

				PacketHeader* header = (PacketHeader*)session->GetRecvBuffer().ReadPos();
				if (dataSize < header->size) break;

				char* packetData = session->GetRecvBuffer().ReadPos();
				
				ClientPacketHandler::HandlePacket(session, packetData, header->id);

				session->GetRecvBuffer().OnRead(header->size);
			}
			session->GetRecvBuffer().Clean();
			session->Recv();
		}
		else if (overlappedEx->type == IO_TYPE::WRITE)
		{
			GLOG(Session, "Session %llu 데이터 송신 완료!", session->GetSessionId());
			session->OnSendCompleted();
		}
	}
}