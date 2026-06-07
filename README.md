# Net-Iocp 서버 실행 및 테스트 가이드

본 프로젝트는 고성능 네트워크 I/O 처리를 위한 IOCP 기반 C++ 서버와, Prometheus/Grafana를 활용한 실시간 메트릭 모니터링 시스템을 포함하고 있습니다.
아래의 순서대로 테스트를 진행하여 서버 동작과 모니터링 연동을 확인하실 수 있습니다.

## 1. 모니터링 시스템 구동 (Docker 필요)
터미널을 열고 `Monitoring` 폴더로 이동한 뒤, 아래 명령어를 통해 Grafana와 Prometheus 컨테이너를 실행합니다.
```powershell
cd Monitoring
docker-compose up -d
```

## 2. 서버 실행
Visual Studio에서 빌드된 독립 실행 파일 `Net-Iocp.exe`를 더블클릭하여 실행합니다. 
(콘솔 창이 뜨며 UDP 9000번 포트에서 수신 대기를 시작합니다.)

## 3. 대시보드 화면 띄우기
웹 브라우저를 열고 다음 주소로 접속합니다.
- **주소:** http://localhost:3000
- **계정:** admin / admin
- 좌측 대시보드 메뉴에서 `netiocp_dashboard`를 엽니다. 이 시점에서는 트래픽이 없어 패킷 카운트가 `0`으로 표시됩니다.

## 4. 🚀 실시간 네트워크 트래픽 테스트 (클라이언트 시뮬레이션)
서버에 수많은 클라이언트 패킷을 전송하는 시뮬레이션을 진행합니다. 새 터미널을 열고 파이썬 스크립트를 실행해 주세요. (시스템에 Python 3.x가 설치되어 있어야 합니다.)

```powershell
python dummy_client.py
```

스크립트가 실행되면 1초에 약 100개의 UDP 패킷이 서버로 전송됩니다.
**이제 띄워두신 Grafana 대시보드 화면을 보시면, 서버가 패킷을 처리함에 따라 패킷 카운트(Packet Count)와 레이턴시(Latency) 그래프가 실시간으로 수직 상승하는 것을 직접 확인하실 수 있습니다!**

## 5. 💡 Docker가 없는 환경에서의 테스트 방법
Docker가 설치되어 있지 않아 Grafana 대시보드를 띄울 수 없는 경우, 아래 방법으로 서버 동작을 확인하실 수 있습니다.
1. `Net-Iocp.exe` 서버와 `dummy_client.py` 스크립트를 실행합니다.
2. 웹 브라우저를 열고 `http://localhost:8080/metrics` 에 접속합니다.
3. 텍스트로 출력되는 `netiocp_packet_count` 수치가 스크립트 전송에 따라 올라가는지 확인합니다. (새로고침 시 수치 증가 확인 가능)

## 6. 종료
- 테스트가 끝나면 스크립트 실행창에서 `Ctrl+C`를 눌러 전송을 중단합니다.
- 모니터링 컨테이너를 끄시려면 `Monitoring` 폴더에서 `docker-compose down`을 입력합니다.
