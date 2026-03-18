#include "pch.h"
#include "NetworkManager.h"
#include "Session.h"
#include "DBManager.h"

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

	cout << "[NetworkManager] 서버 오픈 완료! 포트: " << port << " / 클라이언트 대기 중\n";

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
			cout << "[Session " << session->GetSessionId() << "] "
				<< bytesTransferred << " 바이트의 데이터를 수신했습니다!\n";

			char* recvBuf = session->GetRecvBuffer();
			PacketHeader* header = (PacketHeader*)recvBuf;

			switch (static_cast<EPacketId>(header->id))
			{
			case EPacketId::LoginReq:
			{
				PKT_C2S_LoginReq* loginReq = (PKT_C2S_LoginReq*)recvBuf;
				cout << "[Session] ID:" << loginReq->username << " / PW: " << loginReq->password << "\n";

				int32_t accountId = GDBManager->VerifyAccount(loginReq->username, loginReq->password);
				bool bSuccess = (accountId != -1);

				PKT_S2C_LoginRes loginRes;
				loginRes.header.size = sizeof(PKT_S2C_LoginRes);
				loginRes.header.id = (uint16_t)EPacketId::LoginRes;
				loginRes.bSuccess = bSuccess;
				loginRes.accountId = bSuccess ? accountId : 0;
				
				if (bSuccess)
				{
					shared_ptr<Player> playerProfile = GDBManager->LoadPlayerProfile(accountId);

					if (playerProfile != nullptr)
					{
						loginRes.bHasProfile = true;
						strncpy_s(loginRes.nickname, playerProfile->GetNickname().c_str(), 31);
						cout << " -> 기존 유저 접속! 닉네임: " << loginRes.nickname << "\n";
					}
					else
					{
						loginRes.bHasProfile = false;
						memset(loginRes.nickname, 0, 32); 
						cout << " -> 신규 유저 접속! 닉네임 생성 요청 필요.\n";
					}
				}

				session->Send((char*)&loginRes, loginRes.header.size);

				if (bSuccess) cout << " -> 로그인 성공! 클라이언트에 맵 이동 명령을 하달합니다.\n";
				else cout << " -> 로그인 실패! 클라이언트에 에러를 보냅니다.\n";

				break;
			}
			default:
			{
				cout << "[Session] 알 수 없는 패킷 수신! ID: " << header->id << "\n";
				break;
			}
			}
			session->Recv();
		}
		else if (overlappedEx->type == IO_TYPE::WRITE)
		{
			cout << "[Session " << session->GetSessionId() << "] 데이터 송신 완료!\n";
		}
	}
}
