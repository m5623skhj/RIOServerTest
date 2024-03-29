# RIOBaseServer

## 제작 기간 : 2023.05.09 ~ 2023.07.05

 1. 개요
 2. 패킷 추가 방법
 3. 주요 클래스
 4. 이후 추가된 기능

---

1. 개요

RIO를 사용한 서버입니다.

패킷의 타입을 꺼내와서 분기문으로 나누어 처리하던 것이 불편하여, 클라이언트에서 완성된 패킷을 받았을 때, 검사 및 처리를 하여, 사용자는 패킷 핸들러만 정의하면 되도록 유도하였습니다.

[DBServer](https://github.com/m5623skhj/DBConnector)와 연동하여야만 서버가 올라가며, DBServer가 필요 없을 시, main.cpp에서 DBClient.Start()를 제거해주세요.

현재 프로젝트를 그대로 실행시킬 시, 시작 흐름은 아래와 같은 순서로 진행됩니다.

![Start RIOTestServer   DBClientpng](https://github.com/m5623skhj/RIOServerTest/assets/42509418/58160c6c-0ebf-470b-9890-7461670c3a9e)

아래는 패킷의 송수신의 대략적인 과정입니다.

![RIOBaserServer receive](https://github.com/m5623skhj/RIOServerTest/assets/42509418/5c368a04-99f3-4ce1-a528-a5a72f3e5403)

![RIOBaserServer send](https://github.com/m5623skhj/RIOServerTest/assets/42509418/de342382-2823-4c67-8a4a-4b4cc4b858bd)

---

2. 패킷 추가 방법

Protocol.h에 IPacket을 상속 받아서 패킷 클래스를 정의합니다.

이 때, 각 패킷은 GET_PACKET_ID() 매크로를 반드시 정의해야합니다.

이후 DECLARE_ALL_HANDLER()와 REGISTER_PACKET_LIST()에 생성한 패킷을 등록하고,

PacketHandler.cpp에서

PacketManager::HandlePacket(RIOTestSession& session, 정의한 패킷& packet)의 형태로 패킷 핸들러를 정의해주면 됩니다.

---

3. 주요 클래스

* RIOTestServer
  * 세션의 연결과 IO 처리를 관리하는 클래스
  * Accept 스레드 :
  * 유저들의 연결을 처리하며, 현재 스레드들 중 가장 세션의 수가 적은 스레드에 할당
  * Worker 스레드 :
  * 지정된 주기마다 스레드가 깨어나 현재 완료된 IO가 있는지 확인 및 해당하는 처리 수행
  * 할당된 세션이 하나의 Worker 스레드에 종속적이게 하여, RIO 관련 처리는 락을 사용하지 않고 수행
 
* RIOTestSession
  * 하나의 클라이언트에 해당하는 클래스
  * 내부적으로 현재 진행되고 있는 IO Count로 관리되고 있음
  * IO Count가 0이 되면 연결이 끊긴 것으로 간주하고 세션을 제거함

* IOContext
  * 어떤 타입의 I/O가 완료 되었는지와 해당 I/O를 요청한 세션이 어떤 세션인지에 대해 기록 되어있는 객체
  * contextPool에서 할당 및 할당 해제를 진행함

* PacketManager
  * 패킷의 정보를 저장하고 있는 클래스
  * 패킷 핸들러와 패킷 정보를 가지고 있으며, 이 정보들로 송신 받은 NetBuffer를 유저가 정의한 IPacket 상속 객체로 변환하고, 알맞은 PacketHandler를 호출할 수 있도록 함

* Broadcaster
  * 현재 모든 세션에 대해서 브로드 캐스트를 수행해주는 클래스
  * 세션이 새로 등록될 때, 자동으로 브로드 캐스트 대상으로 등록되며, 세션이 제거되면 자동으로 대상에서 제외됨

* DeadlockChecker
  * 등록된 스레드들에 대해 데드락을 감지하고 정의한 시간 동안 해당 스레드가 갱신되지 않을 시, 강제로 프로그램을 종료시킨다.
  * 현재 Worker들만을 대상으로 하고 있으며, Accepter의 경우, accept()를 blocking 상태로 호출하여 관찰 대상으로 하지 않음

~~* MultiLanClient~~

  ~~* 기존의 [LanClient](https://github.com/m5623skhj/BackupFolder2/tree/master/LanClient)를 수정하여, 하나의 인스턴스에서 여러개의 소켓을 사용하여 다중 클라이언트로 LanServer에 접속할 수 있도록 수정한 클래스~~
  
---

4. 이후 추가된 기능

4.1. 로그
* LogBase를 상속 받은 클래스를 정의하고, 아래 그림과 같이 로깅할 항목들을 나열
* 이후 Logger의 Instance에 WriteLog를 하면 현재 시간(년-월-일_시/UTC)에 해당하는 파일에다 로그 객체를 json 형태로 풀어서 기록함

![image](https://github.com/m5623skhj/RIOServerTest/assets/42509418/08b08090-b160-4c16-a3b8-e6f1b719ae6e)

4.2. 스크립트 로딩
* 지정한 json script들을 로딩하고, 로딩한 데이터들을 키로 찾아올 수 있는 기능
* 필요한 스크립트의 경로는 솔루션 파일 디렉토리를 기준으로 \..\Script에 위치해야 함.
* 필요한 스크립트의 데이터 클래스의 경우, ScriptToObject 프로젝트에 정의.
* 스크립트 데이터에 대한 자세한 사항은 [ScriptToObject](https://github.com/m5623skhj/ScriptToObject)의 데이터 등록 방법을 확인할 것.

4.3. 빌드 출력 파일 경로와 스크립트 데이터
* 빌드 출력 파일 경로는 솔루션 파일 디렉토리를 기준으로 \..\bin\ 에 출력되며, Config.json이 해당 폴더에 위치해야 함.

---

참고 자료

https://github.com/zeliard/RIOTcpServer

https://gist.github.com/ujentus/5997058#file-rioserver_sm9-cpp
