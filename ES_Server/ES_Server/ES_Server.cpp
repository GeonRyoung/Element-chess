#include "pch.h"
#include "DBManager.h"
#include "DataManager.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 7777
#define BUF_SIZE 1024

int main() {
	if (GDBManager->Connect("localhost", 3306, "root", "rjsfud5605!!", "element_auto"))
	{
		GDataManager->Init();
	}
	else
	{
		cerr << "[Error] DB 연결 실패로 서버를 종료합니다." << endl;
		return 1;
	}


	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cerr << "[Error] WSAStartup 실패" << endl;
		return 1;
	}

	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		cerr << "[Error] 소켓 생성 실패" << endl;
		WSACleanup();
		return 1;
	}

	SOCKADDR_IN serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(SERVER_PORT);

	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		cerr << "[Error] Bind 실패" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "[Error] Listen 실패" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	cout << " [Server] 에코 서버가 포트 " << SERVER_PORT << "에서 대기 중입니다..." << endl;

	SOCKADDR_IN clientAddr;
	int clientAddrLen = sizeof(clientAddr);
	SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &clientAddrLen);

	if (clientSocket == INVALID_SOCKET)
	{
		cerr << "[Error] Accept 실패" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	char clientIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
	cout << " [Server] 클라이언트 접속 됨! IP: " << clientIP << endl;

	char buffer[BUF_SIZE];
	int recvLen;

	while (true)
	{
		recvLen = recv(clientSocket, buffer, BUF_SIZE - 1, 0);

		if (recvLen == 0)
		{
			cout << " [Server] 클라이언트가 연결을 정상적으로 종료했습니다." << endl;
			break;
		}
		else if (recvLen == SOCKET_ERROR)
		{
			cerr << " [Error] 데이터 수신 중 에러 발생: " << WSAGetLastError() << endl;
			break;
		}

		if (recvLen < sizeof(PacketHeader))
		{
			cout << "[Warning] 패킷 크기가 너무 작습니다.\n";
			continue;
		}

		PacketHeader* header = (PacketHeader*)buffer;

		switch (header->id)
		{
		case (uint16_t)EPacketId::LoginReq:
		{
			PKT_C2S_LoginReq* loginReq = (PKT_C2S_LoginReq*)buffer;

			cout << " [수신] 로그인 요청 패킷 도착!" << endl;
			cout << "  - ID : " << loginReq->username << endl;
			cout << "  - PW : " << loginReq->password << endl;

			int32_t accountId = GDBManager->VerifyAccount(loginReq->username, loginReq->password);
			
			PKT_S2C_LoginRes resPacket;
			resPacket.header.size = sizeof(PKT_S2C_LoginRes);
			resPacket.header.id = (uint16_t)EPacketId::LoginRes;
			
			if (accountId != -1) {
				cout << " -> DB 확인 완료: 로그인 성공! (AccountID: " << accountId << ")" << endl;
				resPacket.bSuccess = true;
				resPacket.accountId = accountId;
			}
			else {
				cout << " -> DB 확인 완료: 로그인 실패! (정보 불일치)" << endl;
				resPacket.bSuccess = false;
				resPacket.accountId = -1;
			}

			send(clientSocket, (char*)&resPacket, resPacket.header.size, 0);
			cout << " [송신] 로그인 응답 패킷 전송 완료\n";

			break;
		}
		default:
		{
			cout << " [수신] 알 수 없는 패킷 ID: " << header->id << endl;
			break;
		}
		}
	}

	closesocket(clientSocket);
	closesocket(serverSocket);
	WSACleanup();

	cout << " [Server] 서버가 종료되었습니다." << endl;
	return 0;
}