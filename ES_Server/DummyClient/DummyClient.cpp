#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <vector>
#include <string>
// 서버 쪽에서 만든 Packet.h를 복사해서 가져옵니다.
#include "Packet.h" 

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// 가짜 클라이언트 1명의 행동 패턴
void DummyClientTask(int clientId)
{
	// 1. 소켓 생성
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) return;

	// 2. 서버 주소 세팅 (로컬호스트 7777 포트)
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = htons(7777);

	// 3. 서버에 연결 (입장!)
	if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		closesocket(clientSocket);
		return;
	}

	// 4. 로그인 패킷 만들기 (택배 상자 포장)
	PKT_C2S_LoginReq loginPkt;
	loginPkt.header.size = sizeof(PKT_C2S_LoginReq);
	loginPkt.header.id = (uint16_t)EPacketId::LoginReq;

	// 가짜 아이디 비번 생성 (예: user_1, pass_1)
	string userId = "user_" + to_string(clientId);
	string userPw = "pass_" + to_string(clientId);
	strncpy_s(loginPkt.username, userId.c_str(), 31);
	strncpy_s(loginPkt.password, userPw.c_str(), 31);

	// 5. 서버로 융단폭격 발사!
	send(clientSocket, (const char*)&loginPkt, sizeof(loginPkt), 0);

	// 6. 서버가 LoginRes 응답을 줄 때까지 대기
	char recvBuffer[1024];
	recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);

	// 7. 할 일 끝! 접속 종료
	closesocket(clientSocket);
}

int main()
{
	// 윈도우 소켓 초기화
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	cout << "[더미 클라이언트] 500명 동시 접속 테스트를 시작합니다...\n";

	vector<thread> threads;

	// 500명의 가짜 유저(스레드)를 동시에 생성해서 서버로 돌격시킵니다!
	for (int i = 0; i < 5000; ++i)
	{
		threads.emplace_back([i]() { DummyClientTask(i); });
	}

	// 500명이 일을 다 끝낼 때까지 대기
	for (auto& t : threads)
	{
		if (t.joinable())
			t.join();
	}

	cout << "[더미 클라이언트] 폭격 종료! 서버가 살아있는지 확인하세요.\n";

	WSACleanup();
	return 0;
}