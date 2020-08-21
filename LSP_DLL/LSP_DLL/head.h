#pragma once
#include"Debug.h"
#include<WinSock2.h>
#include<WS2spi.h>
#include<Windows.h>
#include<tchar.h>
#pragma comment(lib,"Ws2_32.lib")
LPWSAPROTOCOL_INFOW GetProviderw(LPINT lpnTotalProtocols);
void FreeProvider(LPWSAPROTOCOL_INFO pProtoInfo);