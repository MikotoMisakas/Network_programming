#include"head.h"


//要安装的LSP的硬编码，在移除的时候要需要使用
GUID ProviderGuid = { 0xd3c21122,0x85e1,0x48f3,{0x9a,0xb6,0x23,0xd9,0x0c,0x73,0x07,0xef} };

//将LSP安装到UDP提供者之上
int InstallProvider(WCHAR* wszDllPath) {
	WCHAR wszLSPName[] = L"LSP";//我们LSP的名称
	int nError = NO_ERROR;
	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	WSAPROTOCOL_INFOW UDPLayeredInfo, UDPChainInfo;//要安装的UDP分层协议和协议链
	DWORD dwUdpOrigCatalogId, dwLayeredCatalogId;
	//在Winsock目录中找到原来的UDP服务提供者，我们的lsp要安装在它之上。
	//枚举所有服务提供者
	pProtoInfo = GetProviderw(&nProtocols);//
	for (int  i = 0; i < nProtocols; i++)
	{
		if (pProtoInfo[i].iAddressFamily==AF_INET&&pProtoInfo[i].iProtocol==IPPROTO_UDP)
		{
			memcpy(&UDPChainInfo, &pProtoInfo[i], sizeof(UDPLayeredInfo));
			//去掉XP1_IFS_HANDLES标志
			UDPChainInfo.dwServiceFlags1 = UDPChainInfo.dwServiceFlags1&XP1_IFS_HANDLES;

			//保存原来入口ID
			dwUdpOrigCatalogId = pProtoInfo[i].dwCatalogEntryId;
			break;

		}

	}

	//首先安装分层协议，获取一个Winsock库安排的目录ID号，既dwLayeredCatalogId
	//直接使用下层协议的WSAPROTOCOL_INFOW结构即可
	memcpy(&UDPLayeredInfo, &UDPChainInfo, sizeof(UDPLayeredInfo));

	//修改协议的名称、类型设置为PFL_HIDDEN标志
	wcscpy(UDPLayeredInfo.szProtocol, wszLSPName);
	UDPLayeredInfo.ProtocolChain.ChainLen = LAYERED_PROTOCOL;
	UDPLayeredInfo.dwProviderFlags |= PFL_HIDDEN;

	//安装
	if (::WSCInstallProvider(&ProviderGuid, wszDllPath, &UDPLayeredInfo, 1, &nError) == SOCKET_ERROR)
	{
		return nError;
	}

	//重新枚举协议获取分层协议的目录ID号
	FreeProvider(pProtoInfo);
	pProtoInfo = GetProviderw(&nProtocols);
	for (int i = 0; i < nProtocols; i++)
	{
		if (memcpy(&pProtoInfo[i].ProviderId,&ProviderGuid,sizeof(ProviderGuid))==0)
		{
			dwLayeredCatalogId = pProtoInfo[i].dwCatalogEntryId;
			break;
		}
	}
	
	//安装协议链、修改协议类型
	WCHAR wszChainName[WSAPROTOCOL_LEN + 1];
	swprintf(wszChainName, L"%ws over %ws", wszLSPName, UDPChainInfo.szProtocol);
	wcscpy(UDPChainInfo.szProtocol, wszChainName);
	if (UDPChainInfo.ProtocolChain.ChainLen==1)
	{
		UDPChainInfo.ProtocolChain.ChainEntries[1] = dwUdpOrigCatalogId;

	}
	else
	{
		int i = 0;
		for ( i = UDPChainInfo.ProtocolChain.ChainLen; i >0; i--)
		{
			UDPChainInfo.ProtocolChain.ChainEntries[i] = UDPChainInfo.ProtocolChain.ChainEntries[i - 1];
		}
	}
	
	UDPChainInfo.ProtocolChain.ChainLen++;
	//将我们的分层一些置于此协议的链的顶端
	UDPChainInfo.ProtocolChain.ChainEntries[0] = dwLayeredCatalogId;

	//获取一个Guid并安装
	GUID ProviderChainGuid;
	if (::UuidCreate(&ProviderChainGuid)==RPC_S_OK)
	{
		if (::WSCInstallProvider(&ProviderChainGuid,wszDllPath,&UDPChainInfo,1,&nError)==SOCKET_ERROR)
		{
			return nError;

		}


	}
	else
	{
		return GetLastError();
	}

	//重新排序Winsock目录，将我们的协议链提前
	//重新枚举安装协议
	FreeProvider(pProtoInfo);
	pProtoInfo = GetProviderw(&nProtocols);
	DWORD dwIds[20];
	int nIndex = 0;

	//添加我们的协议链
	for (int i = 0; i < nProtocols; i++)
	{
		if ((pProtoInfo[i].ProtocolChain.ChainLen>1)&&(pProtoInfo[i].ProtocolChain.ChainEntries[0]!=dwLayeredCatalogId))
		{
			dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
		}
	}

	//添加其他协议
	for (int  i = 0; i < nProtocols; i++)
	{
		if ((pProtoInfo[i].ProtocolChain.ChainLen<=1)||(pProtoInfo[i].ProtocolChain.ChainEntries[0]!=dwLayeredCatalogId))
		{
			dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
		}

	}

	//重新排序Winsock目录
	nError = ::WSCWriteProviderOrder(dwIds, nIndex);
	FreeProvider(pProtoInfo);
	return nError;


}

//移除LSP
//移除LSP的函数是WSCDeinstallProvider,只要为他传递要移除的提供者的GUID即可
//移除LSP时，要先根据分层协议的GUID号找到其目录ID号，然后逐步移除各协议链，最后在移除分层协议的提供者。
void RemoveProvider() {
	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	DWORD dwLayeredCatalogId;
	
	//根据Guid取得分层协议的目录ID号
	pProtoInfo = GetProviderw(&nProtocols);
	int nError;
	for (int  i = 0; i < nProtocols; i++)
	{
		if (memcmp(&ProviderGuid,&pProtoInfo[i].ProviderId,sizeof(ProviderGuid))==0)
		{
			dwLayeredCatalogId = pProtoInfo[i].dwCatalogEntryId;
			break;
		}
		if (i < nProtocols)
		{
			for ( i = 0; i < nProtocols; i++)
			{
				if ((pProtoInfo->ProtocolChain.ChainLen>1)&&(pProtoInfo[i].ProtocolChain.ChainEntries[0]==dwLayeredCatalogId))
				{
					::WSCDeinstallProvider(&pProtoInfo[i].ProviderId, &nError);

				}

			}
			::WSCDeinstallProvider(&ProviderGuid, &nError);
		}
	}





}


int main() {

	WCHAR LSP[] =L"C:\\Users\\konglinghui\\Desktop\\report\\LSP.DLL";
	InstallProvider(LSP);



}