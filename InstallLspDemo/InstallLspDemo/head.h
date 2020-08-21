#pragma once
#include<stdio.h>
#include<WinSock2.h>
#include<winsock.h>
#include <shlwapi.h>
#include "Ws2tcpip.h"
#include<windows.h>
#include<WS2spi.h>
#include"iphlpapi.h"
#include<SpOrder.h>
#pragma comment(lib,"Rpcrt4.lib")//实现了UuidCreate函数
#pragma comment(lib,"Iphlpapi.lib")

//#include<WinSock2.h>//init sockÎÄ¼þ
#pragma comment(lib,"WS2_32.lib")//
LPWSAPROTOCOL_INFOW GetProviderw(LPINT lpnTotalProtocols);
void FreeProvider(LPWSAPROTOCOL_INFO pProtoInfo);