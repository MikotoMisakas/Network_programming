#include"ClientSock.h"




CInitSock::CInitSock(BYTE minorVer, BYTE majorVer) {




	//≥ı ºªØWS2_32.dll
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(minorVer, majorVer);
	if (::WSAStartup(sockVersion, &wsaData) != 0)
	{
		exit(0);
	}



}

CInitSock::~CInitSock() {

	::WSACleanup();

}

