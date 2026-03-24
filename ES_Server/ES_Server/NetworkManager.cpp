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

			if (session->GetRecvBuffer().OnWrite(bytesTransferred) == false)
			{
				session->Disconnect();
				continue;
			}

			while (true)
			{
				int32_t dataSize = session->GetRecvBuffer().DataSize();
				if (dataSize < sizeof(PacketHeader))
					break;

				PacketHeader* header = (PacketHeader*)session->GetRecvBuffer().ReadPos();
				if (dataSize < header->size)
					break;

				char* packetData = session->GetRecvBuffer().ReadPos();
				switch (static_cast<EPacketId>(header->id))
				{
				case EPacketId::LoginReq:
				{
					PKT_C2S_LoginReq* loginReq = (PKT_C2S_LoginReq*)packetData;
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
						session->SendAccountId(accountId);

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
				case EPacketId::CreateNicknameReq:
				{
					PKT_C2S_CreateNicknameReq* req = (PKT_C2S_CreateNicknameReq*)packetData;
					cout << "[Session " << session->GetSessionId() << "] 닉네임 생성 요청: " << req->nickname << "\n";

					bool bSuccess = false;

					int32_t accountId = session->GetAccountId();

					if (accountId != 0)
					{
						bSuccess = GDBManager->CreatePlayerProfile(accountId, req->nickname);
					}
					else
					{
						cout << " -> 에러: 로그인되지 않은 유저의 생성 요청입니다!\n";
					}

					PKT_S2C_CreateNicknameRes res;
					res.header.size = sizeof(PKT_S2C_CreateNicknameRes);
					res.header.id = (uint16_t)EPacketId::CreateNicknameRes;
					res.bSuccess = bSuccess;

					session->Send((char*)&res, res.header.size);
					if (bSuccess) cout << " -> 닉네임 생성 및 DB 저장 성공!\n";
					break;
				}
				case EPacketId::EnterGameReq:
				{
					PKT_C2S_EnterGameReq* req = (PKT_C2S_EnterGameReq*)packetData;
					cout << "[Session] 게임 시작 요청 " << "\n";

					bool bSuccess = false;

					//[TO DO] 게임 데이터 로드

					PKT_S2C_EnterGameRes res;
					res.header.size = sizeof(PKT_S2C_EnterGameRes);
					res.header.id = (uint16_t)EPacketId::EnterGameRes;
					res.bSuccess = bSuccess;

					session->Send((char*)&res, res.header.size);
					if (bSuccess) cout << "게임 사작 성공!\n";
					break;
				}
				case EPacketId::RefreshShopReq :
				{
					PKT_C2S_RefreshShopReq* req = (PKT_C2S_RefreshShopReq*)packetData;
					cout << "[Session " << session->GetSessionId() << "] 상점 리롤 요청 수신!\n";

					PKT_S2C_RefreshShopRes res;
					res.header.size = sizeof(PKT_S2C_RefreshShopRes);
					res.header.id = (uint16_t)EPacketId::CreateNicknameRes;

					int32_t currentGold = 10;
					int32_t rerollCost = 2;

					if (currentGold >= rerollCost)
					{
						res.bSuccess = true;
						res.remainGold = currentGold - rerollCost;

						for (int i = 0; i < 5; i++)
						{
							res.shopUnits[i] = (rand() % 54) + 1;
						}

						cout << " -> 리롤 성공! 남은 골드: " << res.remainGold << " / 뽑힌 유닛: ";
						for (int i = 0; i < 5; ++i) cout << res.shopUnits[i] << " ";
						cout << "\n";
					}
					else
					{
						cout << " -> 리롤 실패: 골드가 부족합니다!\n";
						res.remainGold = currentGold; // 돈이 없으니 그대로 돌려줌
					}
				}
				default:
				{
					cout << "[Session] 알 수 없는 패킷 수신! ID: " << header->id << "\n";
					break;
				}
				}
				session->GetRecvBuffer().OnRead(header->size);
			}
			session->GetRecvBuffer().Clean();
			session->Recv();
		}
		else if (overlappedEx->type == IO_TYPE::WRITE)
		{
			cout << "[Session " << session->GetSessionId() << "] 데이터 송신 완료!\n";
			session->OnSendCompleted();
		}
	}
}

