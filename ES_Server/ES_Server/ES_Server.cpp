#include "pch.h"
#include "DBManager.h"
#include "DataManager.h"

 int main() {
    // 1. DB 연결
    if (GDBManager->Connect("localhost", 3306, "root", "rjsfud5605!!", "element_auto")) {
        // 2. 마스터 데이터 캐싱 (전부 가져오기)
        GDataManager->Init();

        // 3. 로그인 시뮬레이션 (일부 가져오기)
        int32_t accId = GDBManager->VerifyAccount("park", "1234");
        if (accId != -1) {
            auto player = GDBManager->LoadPlayerProfile(accId);
            if (player) cout << " [Login Success] Welcome, " << player->GetNickname() << endl;
        }
    }
    GDBManager->Disconnect();
    return 0;
}