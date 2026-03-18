#include "pch.h"
#include "DBManager.h"
#include "DataManager.h"
#include "NetworkManager.h"
#include <locale>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 7777

int main() {
	SetConsoleOutputCP(CP_UTF8);

	// 1. DB 연결 및 데이터 초기화 (기존 코드 유지)
	if (GDBManager->Connect("localhost", 3306, "root", "rjsfud5605!!", "element_auto"))
	{
		GDataManager->Init();
	}
	else
	{
		cerr << "[Error] DB 연결 실패로 서버를 종료합니다." << endl;
		return 1;
	}

	cout << "=========================================" << endl;
	cout << "       Element Defense Server Start      " << endl;
	cout << "=========================================" << endl;

	// 2. 네트워크 매니저 생성 및 서버 가동!
	// (기존의 길고 복잡했던 네트워크 초기화와 무한 루프가 단 두 줄로 압축됩니다)
	NetworkManager netManager;

	// StartServer 함수 내부에서 while(true)로 accept를 돌기 때문에,
	// 서버가 켜져 있는 동안 메인 스레드는 여기서 계속 머물게 됩니다.
	if (!netManager.StartServer(SERVER_PORT))
	{
		cerr << "[Error] 네트워크 서버 시작 실패!" << endl;
		return 1;
	}

	// 정상적인 무한 루프가 돌아간다면 이 코드는 실행되지 않습니다.
	cout << "[System] 서버가 종료되었습니다." << endl;
	return 0;
}