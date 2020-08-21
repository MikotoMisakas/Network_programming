// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include"head.h"

WSPUPCALLTABLE g_pUpCallTable;//上层函数列表，如果LSP创建了自己的伪句柄，才能使用这个函数列表
WSPPROC_TABLE g_NextProcTable;//下层函数列表
TCHAR g_szCurrentApp[MAX_PATH];//当前调用本DLL的程序的名称



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
	case DLL_PROCESS_ATTACH: {
		::GetModuleFileName(NULL, g_szCurrentApp, MAX_PATH);
	}
							 break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

int WSPAPI WSPSendTo(

	SOCKET s,
	LPWSABUF lpBuffers,
	DWORD dwBufferCount,
	LPDWORD lpNumberOfBytesSend,
	DWORD dwFlags,
	const struct sockaddr FAR* lpTo,
	int iTolen,
	LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	LPWSATHREADID lpThreadId,
	LPINT lpErrno
) {
	ODS1(L"query send to %s", g_szCurrentApp);
	return g_NextProcTable.lpWSPSendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSend, dwFlags, lpTo, iTolen, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);
}


int WSPAPI WSPStartup(
	WORD wVersionRequested,
	LPWSPDATA lpWSPData,
	LPWSAPROTOCOL_INFO lpProtocolInfo,
	WSPUPCALLTABLE UpcallTable,
	LPWSPPROC_TABLE lpProcTable
) {
	ODS1(L" WSPStartup .....%s \n", g_szCurrentApp);
	if (lpProtocolInfo->ProtocolChain.ChainLen<=1)
	{
		return WSAEPROVIDERFAILEDINIT;
	}

	//保存向上调用的函数表指针
	g_pUpCallTable = UpcallTable;

	//枚举协议，找到下层协议的WSAPROTOCOL_INFOW结构
	WSAPROTOCOL_INFOW NextProtocolInfo;
	int nTotalProtos;
	LPWSAPROTOCOL_INFOW pProtoInfo = GetProviderw(&nTotalProtos);

	//下层入口ID
	DWORD dwBaseEntryId = lpProtocolInfo->ProtocolChain.ChainEntries[1];
	for (int  i = 0; i < nTotalProtos; i++)
	{
		if (pProtoInfo[i].dwCatalogEntryId==dwBaseEntryId)
		{
			memcpy(&NextProtocolInfo, &pProtoInfo[i], sizeof(NextProtocolInfo));
			break;

		}
		if (i >= nTotalProtos)
		{
			ODS(L"WSPStartup:	Can not find underlying protocol\n");
			return WSAEPROVIDERFAILEDINIT;

		}

	}

	//加载下层协议的DLL
	int nError;
	TCHAR szBaseProviderDll[MAX_PATH];
	int nLen = MAX_PATH;

	//取得下层提供者DLL路径
	if (::WSCGetProviderPath(&NextProtocolInfo.ProviderId,szBaseProviderDll,&nLen,&nError)==SOCKET_ERROR)
	{
		ODS1(L"WSPStartup WSCGetProviderPath() failed %d\n", nError);
		return WSAEPROVIDERFAILEDINIT;
	}

	if (!::ExpandEnvironmentStrings(szBaseProviderDll,szBaseProviderDll,MAX_PATH))
	{
		ODS(L"WSPStartup ExpandEnvironmentStrings() failed %d\n", nError);
		return WSAEPROVIDERFAILEDINIT;
	}


	//加载下层提供者
	HMODULE hModule = ::LoadLibrary(szBaseProviderDll);
	if (hModule==NULL)
	{
		ODS1(L"WSPStartup LoadLibrary() failed %d\n", ::GetLastError());
		return WSAEPROVIDERFAILEDINIT;
	}

	//导入下层提供者的WSPStartup函数
	LPWSPSTARTUP pfnWSpsTARTUP = NULL;
	pfnWSpsTARTUP = (LPWSPSTARTUP)::GetProcAddress(hModule, "WSPStartup");
	if (pfnWSpsTARTUP==NULL)
	{
		ODS1(L"WSPStartup GetProcaddress() failed%d\n", ::GetLastError());
		return WSAEPROVIDERFAILEDINIT;

	}

	//调用下层提供者的WSPStartup函数
	LPWSAPROTOCOL_INFOW pInfo = lpProtocolInfo;
	if (NextProtocolInfo.ProtocolChain.ChainLen==BASE_PROTOCOL)
	{
		pInfo = &NextProtocolInfo;

	}

	int nRet = pfnWSpsTARTUP(wVersionRequested, lpWSPData, pInfo, UpcallTable, lpProcTable);
	if (nRet!=ERROR_SUCCESS)
	{
		ODS1(L"WSPStartup underlying provider WSPStartup() failed %d\n", nRet);
		return nRet;

	}
	  //保存下层提供者的函数表
	g_NextProcTable = *lpProcTable;

	//修改传递给上层的函数表,Hook感兴趣的函数，这里作为示例，仅HOOK了WSPSendTo函数
	//还可以HOOK其他函数，如WSPSocket、WSPCloseSocket、WSPConnect等
	lpProcTable->lpWSPSendTo = WSPSendTo;
	FreeProvider(pProtoInfo);
	return nRet;
	







}

