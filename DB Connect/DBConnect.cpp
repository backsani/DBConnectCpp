#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <Windows.h>
#include "C:\Bitnami\wampstack-8.1.2-0\mariadb\include\mysql\mysql.h"

using namespace std;

enum class UserStatus
{
	Main,
	Login,
	Signup
};

int main()
{
	MYSQL * conn;

	MYSQL_STMT* SignupStmt;
	MYSQL_BIND SignupBind[3];

	MYSQL_STMT* LoginStmt;
	MYSQL_BIND LoginBind[2];

	MYSQL_RES * res;
	MYSQL_ROW row;

	UserStatus currentStatus = UserStatus::Main;

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

	char gameId[21] = { 0 };
	char gamePassword[21] = { 0 };
	char gameName[21] = { 0 };

	unsigned long gameIdLength;
	unsigned long gamePasswordLength;
	unsigned long gameNameLength;

	const char* SignupQuery = "INSERT INTO Player_List(name, id, password) VALUES (?, ?, ?)";

	if (mysql_stmt_prepare(SignupStmt, SignupQuery, strlen(SignupQuery))) {
		std::cerr << "mysql_SignupStmt_prepare() failed\n";
		std::cerr << mysql_stmt_error(SignupStmt) << std::endl;
		mysql_stmt_close(SignupStmt);
		mysql_close(conn);
		return EXIT_FAILURE;
	}

	const char* LoginQuery = "SELECT id, password FROM Player_List WHERE id = ?";

	if (mysql_stmt_prepare(LoginStmt, LoginQuery, strlen(LoginQuery))) {
		std::cerr << "mysql_LoginStmt_prepare() failed\n";
		std::cerr << mysql_stmt_error(LoginStmt) << std::endl;
		mysql_stmt_close(LoginStmt);
		mysql_close(conn);
		return EXIT_FAILURE;
	}

	while (1)
	{
		system("cls");
		memset(gameId, 0, sizeof(gameId));
		memset(gameId, 0, sizeof(gamePassword));
		memset(gameId, 0, sizeof(gameName));

		if (currentStatus == UserStatus::Main)
		{
			int selectStatus = 0;
			cout << "로그인하기 : 1 \n회원가입하기 : 2\n";
			while (!(cin >> selectStatus)) {
				std::cout << "로그인이나 회원가입을 위해 1,2를 입력해주세요! \n" << std::endl;
				std::cin.clear(); // 스트림의 상태를 초기화
				std::cin.ignore(10000, '\n'); // 잘못된 입력을 버퍼에서 제거
				std::cout << "로그인하기 : 1 \n회원가입하기 : 2\n"; // 다시 입력 요청
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

			cout << "--------------------------------\n--          로 그 인          --\n--------------------------------\n" << endl;
			cout << "아이디를 입력해주세요(20자) : ";
			std::cin >> gameId;
			cout << "비밀번호를 입력해주세요(20자) : ";
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
				//if (mysql_stmt_errno(LoginStmt) == 1062)				//errno로 에러 코드를 받아온다. 1062는 이미 존재하는 프라이머리 키를 삽입시도 시 발생한다.
				//{
				//	cerr << "이미 존재하는 아이디입니다." << endl;
				//}
				//else
				//{
				//	cerr << "mysql_stmt_execute() failed \n";
				//	cerr << mysql_stmt_error(SignupStmt) << endl;
				//}
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

			// 결과를 가져옴
			if((IsLogin = mysql_stmt_fetch(LoginStmt)) == 0) {
				std::cout << "User ID: " << gameId << std::endl;
				std::cout << "User Password: " << gamePassword << std::endl;

				IsLogin = true;
			}
			if (IsLogin == MYSQL_NO_DATA)
			{
				cout << gameId << " : 해당 아이디가 존재하지 않습니다." << endl;
			}
			else if (!strcmp(gamePasswordCopy, gamePassword))
			{
				cout << "로그인에 성공하였습니다." << endl;
			}
			else {
				cout << "비밀번호가 일치하지 않습니다." << endl;
			}
			
			cin >> IsLogin;
		}

		else if (currentStatus == UserStatus::Signup)
		{
			cout << "--------------------------------\n--          회원가입          --\n--------------------------------\n" << endl;
			cout << "아이디를 입력해주세요(20자) : ";
			std::cin >> gameId;
			cout << "비밀번호를 입력해주세요(20자) : ";
			std::cin >> gamePassword;
			cout << "닉네임을 입력해주세요(20자) : ";
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
				if (mysql_stmt_errno(SignupStmt) == 1062)				//errno로 에러 코드를 받아온다. 1062는 이미 존재하는 프라이머리 키를 삽입시도 시 발생한다.
				{
					cerr << "이미 존재하는 아이디입니다." << endl;
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


