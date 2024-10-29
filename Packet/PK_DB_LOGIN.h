#pragma once
#include "Packet.h"

class PK_DB_LOGIN : public Packet
{

public:
	PK_DB_LOGIN();
	int Serialaze(char* buffer, int BufLength);
	int DeSerialaze(char* buffer);
};

