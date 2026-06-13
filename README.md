# Net-Iocp 서버 실행 및 테스트 가이드

본 프로젝트는 고성능 네트워크 I/O 처리를 위한 IOCP 기반 C++ 서버와, Prometheus/Grafana를 활용한 실시간 메트릭 모니터링 시스템을 포함하고 있습니다.
아래의 순서대로 테스트를 진행하여 서버 동작과 모니터링 연동을 확인하실 수 있습니다.

## 1. 모니터링 시스템 구동 (Docker 필요)
터미널을 열고 `Monitoring` 폴더로 이동한 뒤, 아래 명령어를 통해 Grafana와 Prometheus 컨테이너를 실행합니다.
```powershell
cd Net-Iocp\Monitoring
docker-compose up -d
```

## 2. 서버 실행
Visual Studio에서 빌드된 독립 실행 파일 `Net-Iocp.exe`를 더블클릭하여 실행합니다. 
(콘솔 창이 뜨며 UDP 9000번 포트에서 수신 대기를 시작합니다.)

## 3. 대시보드 화면 띄우기
웹 브라우저를 열고 다음 주소로 접속합니다.
- **주소:** http://localhost:3000
- **계정:** admin / admin
- 좌측 대시보드 메뉴에서 `netiocp_dashboard`를 엽니다. 이제 **Global Metrics**, **Network Performance**, **System Resource** 3단 레이아웃으로 구성된 상세 대시보드를 보실 수 있습니다. 초기에는 접속자가 없어 지표가 `0`으로 표시됩니다.

## 4. 🚀 완전 자동화된 실시간 서버 부하 테스트 (C++ 내부 더미 클라이언트)
이전 버전에서는 파이썬 기반의 `dummy_client.py`를 사용했지만, 현재의 **최종 완성 버전**에서는 서버 자체에 고성능 C++ 기반의 `BotManager`가 내장되어 있습니다.

서버가 실행됨과 동시에, 내부적으로 **100개의 독립된 C++ 가상 유저(Bot)** 가 각각 고유한 클라이언트 UDP 소켓을 열어 서버(포트 9000)로 쉴 새 없이 `CS_POSITION_UPDATE` 이동 패킷을 쏘아댑니다.
- **아키텍처 증명:** 100명의 봇이 각자의 `IP:Port`를 통해 통신하면, 서버의 `SessionManager`가 이를 정확하게 식별하고 100개의 `RudpSession`으로 라우팅(멀티플렉싱)합니다.
- **RUDP 신뢰성 검증:** 각 패킷은 개별 RUDP 시퀀스를 통해 추적되며, 서버는 각 봇의 원래 주소로 `WSASendTo`를 통해 정확하게 `RUDP_ACK` 및 브로드캐스트 위치 데이터를 되돌려 보냅니다.

**서버(Net-Iocp.exe)만 실행하시면, 별도의 스크립트 구동 없이 띄워두신 Grafana 대시보드 화면에서 즉각적으로 Active Sessions가 100으로 증가하고 PPS(Packets Per Second) 지표가 치솟는 것을 직접 확인하실 수 있습니다!**

## 5. 💡 외부 스크립트 연동 테스트 (선택 사항)
만약 이전처럼 파이썬 기반의 대화형 CLI 클라이언트를 통해 부하를 수동으로 조작해보고 싶으시다면, 기존의 `dummy_client.py`도 여전히 완벽하게 호환 및 작동합니다.
1. `Net-Iocp.exe` 서버를 먼저 띄워둡니다 (내부 C++ 봇 100마리가 이미 접속함)
2. 새 터미널에서 `pip install rich prompt_toolkit` 후 `python dummy_client.py` 실행
3. `start_bots 500` 명령어를 입력하면 외부 파이썬 더미 500개가 추가로 서버에 접속하여, 총 600개의 세션이 멀티플렉싱되는 엄청난 광경을 보실 수 있습니다!

## 6. 종료
- 테스트가 끝나면 스크립트 실행창에서 `Ctrl+C`를 눌러 전송을 중단합니다.
- 모니터링 컨테이너를 끄시려면 `Net-Iocp\Monitoring` 폴더에서 `docker-compose down`을 입력합니다.
