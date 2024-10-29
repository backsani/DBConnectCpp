#include "DBServer.h"

DBServer::DBServer()
{
	this->packet.push_back(std::make_unique<PK_MESSAGE>());
	this->packet.push_back(std::make_unique<PK_TIME>());
	this->packet.push_back(std::make_unique<PK_DB_LOGIN>());

	if (DBInit() == 1)
	{
		return;
	}
}

bool DBServer::InitSocket()
{
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		printf("[에러] WSAStartup() 함수 실패 : %d\n", WSAGetLastError());
		return false;
	}

	listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (listenSocket == INVALID_SOCKET)
	{
		printf("[에러] WSASocket() 함수 실패 : %d\n", WSAGetLastError());
		return false;
	}

	printf("소켓 초기화 성공\n");
	return true;
}

bool DBServer::BindSocket()
{
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT); //서버 포트를 설정한다.				
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//위에서 지정한 서버 주소 정보와 cIOCompletionPort 소켓을 연결한다.
	if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR_IN)))
	{
		printf("[에러] bind()함수 실패 : %d\n", WSAGetLastError());
		return false;
	}

	if (listen(listenSocket, 5))
	{
		printf("[에러] listen()함수 실패 : %d\n", WSAGetLastError());
		return false;
	}

	printf("서버 등록 성공..\n");
	return true;
}

bool DBServer::StartServer(const UINT32 maxGameServerCount)
{
	CreateGameServer(maxGameServerCount);

	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (mIOCPHandle == NULL)
	{
		printf("[에러] CreateIoCompletionPort() 함수 실패: %d\n", GetLastError());
		return false;
	}

	if (!CreateWokerThread()) {
		printf("[에러] CreateWokerThread() 함수 실패: %d\n", GetLastError());
		return false;
	}

	AccepterGameServer();

	printf("서버 시작\n");
	return true;
}

void DBServer::DestroyThread()
{
	mIsWorkerRun = false;
	CloseHandle(mIOCPHandle);

	for (auto& th : mWorkerThreads)
	{
		if (th.joinable())
		{
			th.join();
		}
	}

	mIsAccepterRun = false;
	closesocket(listenSocket);

	if (mAccepterThread.joinable())
	{
		mAccepterThread.join();
	}
}

void DBServer::CreateGameServer(const UINT32 maxGameServerCount)
{
	for (UINT32 i = 0; i < maxGameServerCount; i++)
	{
		mGameServerInfos.emplace_back();
	}
}

bool DBServer::CreateWokerThread()
{
	unsigned int uiThreadId = 0;
	//WaingThread Queue에 대기 상태로 넣을 쓰레드들 생성 권장되는 개수 : (cpu개수 * 2) + 1 
	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		mWorkerThreads.emplace_back([this]() { WokerThread(); });
	}

	printf("WokerThread 시작..\n");
	return true;
}

void DBServer::WokerThread()
{
	GameServerInfo* pGameServerInfo = nullptr;
	BOOL bSuccess = TRUE;
	DWORD dwIoSize = 0;
	LPOVERLAPPED lpOverlapped = NULL;
	PK_Data currentHeader;
	Buffer_Converter bufferCon;
	int bufSize = 0;


	while (mIsWorkerRun)
	{
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
			&dwIoSize,					// 실제로 전송된 바이트
			(PULONG_PTR)&pGameServerInfo,		// CompletionKey
			&lpOverlapped,				// Overlapped IO 객체
			INFINITE);					// 대기할 시간

		//사용자 쓰레드 종료 메세지 처리..
		if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			mIsWorkerRun = false;
			continue;
		}

		if (NULL == lpOverlapped)
		{
			continue;
		}

		//client가 접속을 끊었을때..			
		if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
		{
			printf("socket(%d) 접속 끊김\n", (int)pGameServerInfo->_gameServerClient);
			CloseSocket(pGameServerInfo);
			continue;
		}


		auto pOverlappedEx = (mOverlappedEx*)lpOverlapped;

		currentHeader = bufferCon.GetHeader(pGameServerInfo->mRecvBuf);

		//Overlapped I/O Recv작업 결과 뒤 처리
		if (IOOperation::RECV == pOverlappedEx->_operation)
		{
			bufSize = packet[currentHeader]->DeSerialaze(pGameServerInfo->mRecvBuf);
			strcpy(pGameServerInfo->mRecvBuf, packet[currentHeader]->GetBuffer());

			printf("[수신] bytes : %d , msg : %s\n", dwIoSize, pGameServerInfo->mRecvBuf);

			if (currentHeader == MESSAGE)
			{
				//브로드캐스트 구현 실패
				/*int bufLength = packet[currentHeader]->Serialaze(pGameServerInfo->mRecvBuf, bufSize);
				Broadcast(pGameServerInfo, pGameServerInfo->mRecvBuf, bufLength);*/

			}

			else if (currentHeader == LOGIN)
			{

			}
			
			else if (currentHeader == SIGNUP)
			{

			}

			else
			{
				printf("not exists Packet\n");
				continue;
			}


			//클라이언트에 메세지를 에코한다.
			int bufLength = packet[currentHeader]->Serialaze(pGameServerInfo->mRecvBuf, bufSize);
			//int bufLength = packet[currentHeader]->Serialaze(packet[currentHeader]->GetBuffer());

			SendMsg(pGameServerInfo, pGameServerInfo->mRecvBuf, bufLength);

			BindRecv(pGameServerInfo);
		}
		//Overlapped I/O Send작업 결과 뒤 처리
		else if (IOOperation::SEND == pOverlappedEx->_operation)
		{
			printf("[송신] bytes : %d , msg : %s\n", dwIoSize, packet[currentHeader]->GetBuffer());
		}
		//예외 상황
		else
		{
			printf("socket(%d)에서 예외상황\n", (int)pGameServerInfo->_gameServerClient);
		}
	}
}


void DBServer::AccepterGameServer()
{
	SOCKADDR_IN		stGameServerAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);

	while (mIsAccepterRun)
	{
		//접속을 받을 구조체의 인덱스를 얻어온다.
		GameServerInfo* pGameServerInfo = GetEmptyGameServerInfo();
		if (NULL == pGameServerInfo)
		{
			printf("[에러] Client Full\n");
			return;
		}

		//클라이언트 접속 요청이 들어올 때까지 기다린다.
		pGameServerInfo->_gameServerClient = accept(listenSocket, (SOCKADDR*)&stGameServerAddr, &nAddrLen);
		if (INVALID_SOCKET == pGameServerInfo->_gameServerClient)
		{
			continue;
		}

		printf("게임서버 연결 : SOCKET(%d)\n", (int)pGameServerInfo->_gameServerClient);
	}
}

GameServerInfo* DBServer::GetEmptyGameServerInfo()
{
	for (auto& client : mGameServerInfos)
	{
		if (INVALID_SOCKET == client._gameServerClient)
		{
			return &client;
		}
	}

	return nullptr;
}

bool DBServer::BindIOCompletionPort(GameServerInfo* pGameServerInfo)
{
	//socket과 pGameServerInfo를 CompletionPort객체와 연결시킨다.
	auto hIOCP = CreateIoCompletionPort((HANDLE)pGameServerInfo->_gameServerClient
		, mIOCPHandle
		, (ULONG_PTR)(pGameServerInfo), 0);

	if (NULL == hIOCP || mIOCPHandle != hIOCP)
	{
		printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
		return false;
	}

	return true;
}


//WSARecv Overlapped I/O 작업을 시킨다.
bool DBServer::BindRecv(GameServerInfo* pGameServerInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	pGameServerInfo->_recvOverlappedEx._wsaBuf.len = MAX_SOCKBUF;
	pGameServerInfo->_recvOverlappedEx._wsaBuf.buf = pGameServerInfo->mRecvBuf;
	pGameServerInfo->_recvOverlappedEx._operation = IOOperation::RECV;

	int nRet = WSARecv(pGameServerInfo->_gameServerClient,
		&(pGameServerInfo->_recvOverlappedEx._wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) & (pGameServerInfo->_recvOverlappedEx),
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[에러] WSARecv()함수 실패 : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}

//WSASend Overlapped I/O작업을 시킨다.
bool DBServer::SendMsg(GameServerInfo* pGameServerInfo, char* pMsg, int nLen)
{
	DWORD dwRecvNumBytes = 0;

	//전송될 메세지를 복사
	CopyMemory(pGameServerInfo->mSendBuf, pMsg, nLen);
	pGameServerInfo->mSendBuf[nLen] = '\0';


	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	pGameServerInfo->_sendOverlappedEx._wsaBuf.len = nLen;
	pGameServerInfo->_sendOverlappedEx._wsaBuf.buf = pGameServerInfo->mSendBuf;
	pGameServerInfo->_sendOverlappedEx._operation = IOOperation::SEND;

	int nRet = WSASend(pGameServerInfo->_gameServerClient,
		&(pGameServerInfo->_sendOverlappedEx._wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED) & (pGameServerInfo->_sendOverlappedEx),
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[에러] WSASend()함수 실패 : %d\n", WSAGetLastError());
		return false;
	}
	return true;
}


void DBServer::Broadcast(GameServerInfo* pGameServerInfo, char* buffer, int length)
{
	SOCKET currentClient = pGameServerInfo->_gameServerClient;

	int retval;
	for (auto const clientInfo : mGameServerInfos)
	{
		if (clientInfo._gameServerClient != currentClient)
		{
			//GameServerInfo* pGameServerInfo = (GameServerInfo*)&clientInfo;
			recv(clientInfo._gameServerClient, buffer, length, 0);
		}
	}
}

void DBServer::CloseSocket(GameServerInfo* pGameServerInfo)
{
	bool bIsForce = false;
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER로 설정

	// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
	shutdown(pGameServerInfo->_gameServerClient, SD_BOTH);

	//소켓 옵션을 설정한다.
	setsockopt(pGameServerInfo->_gameServerClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//소켓 연결을 종료 시킨다. 
	closesocket(pGameServerInfo->_gameServerClient);

	pGameServerInfo->_gameServerClient = INVALID_SOCKET;
}



int DBServer::DBInit()
{
	char server[] = "localhost";
	char user[] = "root";
	char password[] = "bitnami";
	char database[] = "test";


	conn = mysql_init(NULL);
	if (conn == NULL) {
		std::cerr << "mysql_init() failed \n";
		return EXIT_FAILURE;
	}

	if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0))
	{
		mysql_close(conn);
		return EXIT_FAILURE;
	}

	SignupStmt = mysql_stmt_init(conn);
	if (!SignupStmt) {
		std::cerr << "mysql_SignupStmt_init() failed \n";
		mysql_close(conn);
		return EXIT_FAILURE;
	}

	LoginStmt = mysql_stmt_init(conn);
	if (!LoginStmt) {
		std::cerr << "mysql_LoginStmt_init() failed \n";
		mysql_close(conn);
		return EXIT_FAILURE;
	}

	if (mysql_stmt_prepare(SignupStmt, SignupQuery, strlen(SignupQuery))) {
		std::cerr << "mysql_SignupStmt_prepare() failed\n";
		std::cerr << mysql_stmt_error(SignupStmt) << std::endl;
		mysql_stmt_close(SignupStmt);
		mysql_close(conn);
		return EXIT_FAILURE;
	}

	if (mysql_stmt_prepare(LoginStmt, LoginQuery, strlen(LoginQuery))) {
		std::cerr << "mysql_LoginStmt_prepare() failed\n";
		std::cerr << mysql_stmt_error(LoginStmt) << std::endl;
		mysql_stmt_close(LoginStmt);
		mysql_close(conn);
		return EXIT_FAILURE;
	}

}