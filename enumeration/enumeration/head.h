#pragma once
#include<stdio.h>
#include<WinSock2.h>
#include<winsock.h>
#include <shlwapi.h>
#include "Ws2tcpip.h"
#include<windows.h>
#include<WS2spi.h>
#include"iphlpapi.h"

#pragma comment(lib,"Iphlpapi.lib")

//#include<WinSock2.h>//init sock文件
#pragma comment(lib,"WS2_32.lib")//链接到WS2_32

LPWSAPROTOCOL_INFOW GetProviderw(LPINT lpnTotalProtocols);