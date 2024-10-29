#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "libmariadb.lib")
#pragma comment(lib, "Packet.lib")

#include <iostream>
#include <string>
#include "DBServer.h"


const UINT16 MAX_GameServer = 3;		//�� �����Ҽ� �ִ� ���Ӽ��� ��

using namespace std;

enum class UserStatus
{
	Main,
	Login,
	Signup
};

int DBInit(MYSQL* conn)
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
}

int main()
{
	DBServer ioCompletionPort;

	//������ �ʱ�ȭ
	ioCompletionPort.InitSocket();

	//���ϰ� ���� �ּҸ� �����ϰ� ��� ��Ų��.
	ioCompletionPort.BindSocket();

	ioCompletionPort.StartServer(MAX_GameServer);

	printf("�ƹ� Ű�� ���� ������ ����մϴ�\n");
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	ioCompletionPort.DestroyThread();
	return 0;

	// -------------------------------------------------------------------------------------------------------\\

	

	
	

	while (1)
	{
		system("cls");
		memset(gameId, 0, sizeof(gameId));
		memset(gameId, 0, sizeof(gamePassword));
		memset(gameId, 0, sizeof(gameName));

		if (currentStatus == UserStatus::Main)
		{
			int selectStatus = 0;
			cout << "�α����ϱ� : 1 \nȸ�������ϱ� : 2\n";
			while (!(cin >> selectStatus)) {
				std::cout << "�α����̳� ȸ�������� ���� 1,2�� �Է����ּ���! \n" << std::endl;
				std::cin.clear(); // ��Ʈ���� ���¸� �ʱ�ȭ
				std::cin.ignore(10000, '\n'); // �߸��� �Է��� ���ۿ��� ����
				std::cout << "�α����ϱ� : 1 \nȸ�������ϱ� : 2\n"; // �ٽ� �Է� ��û
			}

			switch (selectStatus)
			{
			case 1:
				currentStatus = UserStatus::Login;
				break;
			case 2:
				currentStatus = UserStatus::Signup;
				break;
			default:
				break;
			}
		}

		else if (currentStatus == UserStatus::Login)
		{
			char gamePasswordCopy[] = { 0 };

			cout << "--------------------------------\n--          �� �� ��          --\n--------------------------------\n" << endl;
			cout << "���̵� �Է����ּ���(20��) : ";
			std::cin >> gameId;
			cout << "��й�ȣ�� �Է����ּ���(20��) : ";
			std::cin >> gamePasswordCopy;

			strcpy(gamePassword, gamePasswordCopy);

			gameIdLength = strlen(gameId);
			gamePasswordLength = strlen(gamePassword);

			memset(LoginBind, 0, sizeof(LoginBind));
			LoginBind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
			LoginBind[0].buffer = gameId;
			LoginBind[0].buffer_length = sizeof(gameId);
			LoginBind[0].is_null = 0;
			LoginBind[0].length = &gameIdLength;

			mysql_stmt_bind_param(LoginStmt, LoginBind);

			if (mysql_stmt_execute(LoginStmt))
			{
				//if (mysql_stmt_errno(LoginStmt) == 1062)				//errno�� ���� �ڵ带 �޾ƿ´�. 1062�� �̹� �����ϴ� �����̸Ӹ� Ű�� ���Խõ� �� �߻��Ѵ�.
				
			}

			memset(LoginBind, 0, sizeof(LoginBind));

			LoginBind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
			LoginBind[0].buffer = gameId;
			LoginBind[0].buffer_length = sizeof(gameId);
			LoginBind[0].is_null = 0;
			LoginBind[0].length = &gameIdLength;

			LoginBind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
			LoginBind[1].buffer = gamePassword;
			LoginBind[1].buffer_length = sizeof(gamePassword);
			LoginBind[1].is_null = 0;
			LoginBind[1].length = &gamePasswordLength;

			mysql_stmt_bind_result(LoginStmt, LoginBind);

			int IsLogin = false;

			// ����� ������
			if((IsLogin = mysql_stmt_fetch(LoginStmt)) == 0) {
				std::cout << "User ID: " << gameId << std::endl;
				std::cout << "User Password: " << gamePassword << std::endl;

				IsLogin = true;
			}
			if (IsLogin == MYSQL_NO_DATA)
			{
				cout << gameId << " : �ش� ���̵� �������� �ʽ��ϴ�." << endl;
			}
			else if (!strcmp(gamePasswordCopy, gamePassword))
			{
				cout << "�α��ο� �����Ͽ����ϴ�." << endl;
			}
			else {
				cout << "��й�ȣ�� ��ġ���� �ʽ��ϴ�." << endl;
			}
			
			cin >> IsLogin;
		}

		else if (currentStatus == UserStatus::Signup)
		{
			cout << "--------------------------------\n--          ȸ������          --\n--------------------------------\n" << endl;
			cout << "���̵� �Է����ּ���(20��) : ";
			std::cin >> gameId;
			cout << "��й�ȣ�� �Է����ּ���(20��) : ";
			std::cin >> gamePassword;
			cout << "�г����� �Է����ּ���(20��) : ";
			std::cin >> gameName;

			gameIdLength = strlen(gameId);
			gamePasswordLength = strlen(gamePassword);
			gameNameLength = strlen(gameName);

			memset(SignupBind, 0, sizeof(SignupBind));

			SignupBind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
			SignupBind[0].buffer = gameName;
			SignupBind[0].buffer_length = sizeof(gameName);
			SignupBind[0].is_null = 0;
			SignupBind[0].length = &gameNameLength;

			SignupBind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
			SignupBind[1].buffer = gameId;
			SignupBind[1].buffer_length = sizeof(gameId);
			SignupBind[1].is_null = 0;
			SignupBind[1].length = &gameIdLength;

			SignupBind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
			SignupBind[2].buffer = gamePassword;
			SignupBind[2].buffer_length = sizeof(gamePassword);
			SignupBind[2].is_null = 0;
			SignupBind[2].length = &gamePasswordLength;

			if (mysql_stmt_bind_param(SignupStmt, SignupBind)) {
				std::cerr << "mysql_stmt_bind_param() failed \n";
				std::cerr << mysql_stmt_error(SignupStmt) << std::endl;
				mysql_stmt_close(SignupStmt);
				mysql_close(conn);
				return EXIT_FAILURE;
			}

			if (mysql_stmt_execute(SignupStmt)) {
				if (mysql_stmt_errno(SignupStmt) == 1062)				//errno�� ���� �ڵ带 �޾ƿ´�. 1062�� �̹� �����ϴ� �����̸Ӹ� Ű�� ���Խõ� �� �߻��Ѵ�.
				{
					cerr << "�̹� �����ϴ� ���̵��Դϴ�." << endl;
				}
				else
				{
					cerr << "mysql_stmt_execute() failed \n";
					cerr << mysql_stmt_error(SignupStmt) << endl;
				}
			}
			else {
				cout << "Data inserted successfully!" << endl;
			}
		}
		
	}

	mysql_stmt_close(SignupStmt);
	mysql_stmt_close(LoginStmt);
	mysql_close(conn);

	return 0;
	

}

