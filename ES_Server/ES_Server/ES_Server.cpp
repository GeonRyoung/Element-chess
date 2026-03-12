#include "pch.h"

int main() {
    try {
        // 1. 세션 연결 (포트 33060 확인)
        mysqlx::Session sess("localhost", 33060, "root", "rjsfud5605!!");
        mysqlx::Schema db = sess.getSchema("element_chess_db");
        mysqlx::Table unitTable = db.getTable("unit_information");

        // 2. 데이터 조회 (JSON 필드를 포함해 모든 컬럼 선택)
        mysqlx::RowResult res = unitTable.select("unit_id", "name", "unit_stats").execute();

        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "ID\t| 이름\t\t| 상세 스탯 (Raw JSON)" << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;

        // 3. 루프를 돌며 가공 없이 출력
        for (mysqlx::Row row : res.fetchAll()) {
            std::cout << row[0] << "\t| "
                << row[1] << "\t| "
                << row[2] << std::endl; // row[2]가 JSON 필드이며, 그대로 출력됩니다.
        }
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "✅ 총 " << res.count() << "개의 유닛 데이터 로드 완료!" << std::endl;

    }
    catch (const mysqlx::Error& err) {
        std::cerr << "❌ DB 에러: " << err.what() << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "❌ 시스템 예외: " << ex.what() << std::endl;
    }

    return 0;
}