#include "pch.h"
#include "DBManager.h" 

int main() {
    // 1. DB 연결 (비밀번호는 사용자님의 실제 비밀번호로 변경하세요)
    if (GDBManager->Connect("localhost", 33060, "root", "rjsfud5605!!", "element_auto_db")) {
        std::cout << "--- 서버 부팅 완료! DB 연결 성공 ---" << std::endl;

        try {
            // 2. 스키마(데이터베이스) 가져오기
            mysqlx::Schema db = GDBManager->GetSchema();

            // 3. 테이블 접근 및 쿼리 실행
            mysqlx::Table unitTable = db.getTable("unit_information");
            mysqlx::RowResult result = unitTable.select("unit_id", "name", "unit_stats").execute();

            std::cout << "=======================================" << std::endl;
            std::cout << "ID\t이름\t상세 스탯 (JSON)" << std::endl;
            std::cout << "=======================================" << std::endl;

            // 4. 데이터 출력
            for (mysqlx::Row row : result.fetchAll()) {
                std::cout << row[0] << "\t"
                    << row[1] << "\t"
                    << row[2] << std::endl;
            }
            std::cout << "=======================================" << std::endl;
            std::cout << "총 " << result.count() << "개의 유닛 데이터 로드 완료!" << std::endl;

        }
        catch (const std::exception& e) {
            std::cerr << "쿼리 실행 중 에러 발생: " << e.what() << std::endl;
        }
    }
    else {
        std::cerr << "--- DB 연결 실패로 서버를 종료합니다 ---" << std::endl;
    }

    // 5. 안전하게 연결 해제
    GDBManager->Disconnect();
    return 0;
}