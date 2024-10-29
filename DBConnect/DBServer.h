#pragma once
#pragma comment(lib, "ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS

#include <WinSock2.h>
#include <iostream>
#include <vector>
#include <thread>
#include <ctime>
#include <string>

#include "mysql/mysql.h"
#include "PacketSDK.h"

#define PORT 8000
#define MAX_WORKERTHREAD 4
#define MAX_SOCKBUF 256

enum class IOOperation
{
	RECV,
	SEND
};

struct mOverlappedEx
{
	WSAOVERLAPPED _wsaOverlapped;
	SOCKET _gameServerClient;
	WSABUF _wsaBuf;
	IOOperation _operation;
};

struct GameServerInfo
{
	SOCKET _gameServerClient;
	mOverlappedEx _recvOverlappedEx;
	mOverlappedEx _sendOverlappedEx;

	char			mRecvBuf[MAX_SOCKBUF]; //데이터 버퍼
	char			mSendBuf[MAX_SOCKBUF]; //데이터 버퍼

	GameServerInfo()
	{
		ZeroMemory(&_recvOverlappedEx, sizeof(mOverlappedEx));
		ZeroMemory(&_sendOverlappedEx, sizeof(mOverlappedEx));
		_gameServerClient = INVALID_SOCKET;
	}
};

class DBServer
{
private:
	SOCKET listenSocket = INVALID_SOCKET;

	std::vector<std::unique_ptr<Packet>> packet;

	std::vector<GameServerInfo> mGameServerInfos;

	std::vector<std::thread> mWorkerThreads;

	std::thread mAccepterThread;

	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;

	bool mIsWorkerRun = true;

	bool mIsAccepterRun = true;

	char mSocketBuf[1024] = { 0 };


	MYSQL* conn;

	MYSQL_STMT* SignupStmt;
	MYSQL_BIND SignupBind[3];

	MYSQL_STMT* LoginStmt;
	MYSQL_BIND LoginBind[2];

	MYSQL_RES* res;
	MYSQL_ROW row;

	char gameId[21] = { 0 };
	char gamePassword[21] = { 0 };
	char gameName[21] = { 0 };

	unsigned long gameIdLength;
	unsigned long gamePasswordLength;
	unsigned long gameNameLength;

	const char* SignupQuery = "INSERT INTO Player_List(name, id, password) VALUES (?, ?, ?)";
	const char* LoginQuery = "SELECT id, password FROM Player_List WHERE id = ?";

	//UserStatus currentStatus = UserStatus::Main;

public:

	DBServer();

	~DBServer() { WSACleanup(); }

	bool InitSocket();

	bool BindSocket();

	bool StartServer(const UINT32 maxClientCount);

	void DestroyThread();

	void CreateGameServer(const UINT32 maxClientCount);

	bool CreateWokerThread();


	bool BindRecv(GameServerInfo* pGameServerInfo);

	bool SendMsg(GameServerInfo* pGameServerInfo, char* pMsg, int nLen);

private:
	void WokerThread();

	void AccepterGameServer();

	void CloseSocket(GameServerInfo* pGameServerInfo);

	bool BindIOCompletionPort(GameServerInfo* pGameServerInfo);

	GameServerInfo* GetEmptyGameServerInfo();

	void Broadcast(GameServerInfo* pGameServerInfo, char* buffer, int length);

	int DBInit();
};

