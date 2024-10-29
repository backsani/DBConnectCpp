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
		printf("[����] WSAStartup() �Լ� ���� : %d\n", WSAGetLastError());
		return false;
	}

	listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (listenSocket == INVALID_SOCKET)
	{
		printf("[����] WSASocket() �Լ� ���� : %d\n", WSAGetLastError());
		return false;
	}

	printf("���� �ʱ�ȭ ����\n");
	return true;
}

bool DBServer::BindSocket()
{
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT); //���� ��Ʈ�� �����Ѵ�.				
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//������ ������ ���� �ּ� ������ cIOCompletionPort ������ �����Ѵ�.
	if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR_IN)))
	{
		printf("[����] bind()�Լ� ���� : %d\n", WSAGetLastError());
		return false;
	}

	if (listen(listenSocket, 5))
	{
		printf("[����] listen()�Լ� ���� : %d\n", WSAGetLastError());
		return false;
	}

	printf("���� ��� ����..\n");
	return true;
}

bool DBServer::StartServer(const UINT32 maxGameServerCount)
{
	CreateGameServer(maxGameServerCount);

	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (mIOCPHandle == NULL)
	{
		printf("[����] CreateIoCompletionPort() �Լ� ����: %d\n", GetLastError());
		return false;
	}

	if (!CreateWokerThread()) {
		printf("[����] CreateWokerThread() �Լ� ����: %d\n", GetLastError());
		return false;
	}

	AccepterGameServer();

	printf("���� ����\n");
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
	//WaingThread Queue�� ��� ���·� ���� ������� ���� ����Ǵ� ���� : (cpu���� * 2) + 1 
	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		mWorkerThreads.emplace_back([this]() { WokerThread(); });
	}

	printf("WokerThread ����..\n");
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
			&dwIoSize,					// ������ ���۵� ����Ʈ
			(PULONG_PTR)&pGameServerInfo,		// CompletionKey
			&lpOverlapped,				// Overlapped IO ��ü
			INFINITE);					// ����� �ð�

		//����� ������ ���� �޼��� ó��..
		if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			mIsWorkerRun = false;
			continue;
		}

		if (NULL == lpOverlapped)
		{
			continue;
		}

		//client�� ������ ��������..			
		if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
		{
			printf("socket(%d) ���� ����\n", (int)pGameServerInfo->_gameServerClient);
			CloseSocket(pGameServerInfo);
			continue;
		}


		auto pOverlappedEx = (mOverlappedEx*)lpOverlapped;

		currentHeader = bufferCon.GetHeader(pGameServerInfo->mRecvBuf);

		//Overlapped I/O Recv�۾� ��� �� ó��
		if (IOOperation::RECV == pOverlappedEx->_operation)
		{
			bufSize = packet[currentHeader]->DeSerialaze(pGameServerInfo->mRecvBuf);
			strcpy(pGameServerInfo->mRecvBuf, packet[currentHeader]->GetBuffer());

			printf("[����] bytes : %d , msg : %s\n", dwIoSize, pGameServerInfo->mRecvBuf);

			if (currentHeader == MESSAGE)
			{
				//��ε�ĳ��Ʈ ���� ����
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


			//Ŭ���̾�Ʈ�� �޼����� �����Ѵ�.
			int bufLength = packet[currentHeader]->Serialaze(pGameServerInfo->mRecvBuf, bufSize);
			//int bufLength = packet[currentHeader]->Serialaze(packet[currentHeader]->GetBuffer());

			SendMsg(pGameServerInfo, pGameServerInfo->mRecvBuf, bufLength);

			BindRecv(pGameServerInfo);
		}
		//Overlapped I/O Send�۾� ��� �� ó��
		else if (IOOperation::SEND == pOverlappedEx->_operation)
		{
			printf("[�۽�] bytes : %d , msg : %s\n", dwIoSize, packet[currentHeader]->GetBuffer());
		}
		//���� ��Ȳ
		else
		{
			printf("socket(%d)���� ���ܻ�Ȳ\n", (int)pGameServerInfo->_gameServerClient);
		}
	}
}


void DBServer::AccepterGameServer()
{
	SOCKADDR_IN		stGameServerAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);

	while (mIsAccepterRun)
	{
		//������ ���� ����ü�� �ε����� ���´�.
		GameServerInfo* pGameServerInfo = GetEmptyGameServerInfo();
		if (NULL == pGameServerInfo)
		{
			printf("[����] Client Full\n");
			return;
		}

		//Ŭ���̾�Ʈ ���� ��û�� ���� ������ ��ٸ���.
		pGameServerInfo->_gameServerClient = accept(listenSocket, (SOCKADDR*)&stGameServerAddr, &nAddrLen);
		if (INVALID_SOCKET == pGameServerInfo->_gameServerClient)
		{
			continue;
		}

		printf("���Ӽ��� ���� : SOCKET(%d)\n", (int)pGameServerInfo->_gameServerClient);
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
	//socket�� pGameServerInfo�� CompletionPort��ü�� �����Ų��.
	auto hIOCP = CreateIoCompletionPort((HANDLE)pGameServerInfo->_gameServerClient
		, mIOCPHandle
		, (ULONG_PTR)(pGameServerInfo), 0);

	if (NULL == hIOCP || mIOCPHandle != hIOCP)
	{
		printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
		return false;
	}

	return true;
}


//WSARecv Overlapped I/O �۾��� ��Ų��.
bool DBServer::BindRecv(GameServerInfo* pGameServerInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O�� ���� �� ������ ������ �ش�.
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

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[����] WSARecv()�Լ� ���� : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}

//WSASend Overlapped I/O�۾��� ��Ų��.
bool DBServer::SendMsg(GameServerInfo* pGameServerInfo, char* pMsg, int nLen)
{
	DWORD dwRecvNumBytes = 0;

	//���۵� �޼����� ����
	CopyMemory(pGameServerInfo->mSendBuf, pMsg, nLen);
	pGameServerInfo->mSendBuf[nLen] = '\0';


	//Overlapped I/O�� ���� �� ������ ������ �ش�.
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

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[����] WSASend()�Լ� ���� : %d\n", WSAGetLastError());
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
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER�� ����

	// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
	shutdown(pGameServerInfo->_gameServerClient, SD_BOTH);

	//���� �ɼ��� �����Ѵ�.
	setsockopt(pGameServerInfo->_gameServerClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//���� ������ ���� ��Ų��. 
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