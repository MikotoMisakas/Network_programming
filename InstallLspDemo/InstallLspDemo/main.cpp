#include"head.h"


//Ҫ��װ��LSP��Ӳ���룬���Ƴ���ʱ��Ҫ��Ҫʹ��
GUID ProviderGuid = { 0xd3c21122,0x85e1,0x48f3,{0x9a,0xb6,0x23,0xd9,0x0c,0x73,0x07,0xef} };

//��LSP��װ��UDP�ṩ��֮��
int InstallProvider(WCHAR* wszDllPath) {
	WCHAR wszLSPName[] = L"LSP";//����LSP������
	int nError = NO_ERROR;
	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	WSAPROTOCOL_INFOW UDPLayeredInfo, UDPChainInfo;//Ҫ��װ��UDP�ֲ�Э���Э����
	DWORD dwUdpOrigCatalogId, dwLayeredCatalogId;
	//��WinsockĿ¼���ҵ�ԭ����UDP�����ṩ�ߣ����ǵ�lspҪ��װ����֮�ϡ�
	//ö�����з����ṩ��
	pProtoInfo = GetProviderw(&nProtocols);//
	for (int  i = 0; i < nProtocols; i++)
	{
		if (pProtoInfo[i].iAddressFamily==AF_INET&&pProtoInfo[i].iProtocol==IPPROTO_UDP)
		{
			memcpy(&UDPChainInfo, &pProtoInfo[i], sizeof(UDPLayeredInfo));
			//ȥ��XP1_IFS_HANDLES��־
			UDPChainInfo.dwServiceFlags1 = UDPChainInfo.dwServiceFlags1&XP1_IFS_HANDLES;

			//����ԭ�����ID
			dwUdpOrigCatalogId = pProtoInfo[i].dwCatalogEntryId;
			break;

		}

	}

	//���Ȱ�װ�ֲ�Э�飬��ȡһ��Winsock�ⰲ�ŵ�Ŀ¼ID�ţ���dwLayeredCatalogId
	//ֱ��ʹ���²�Э���WSAPROTOCOL_INFOW�ṹ����
	memcpy(&UDPLayeredInfo, &UDPChainInfo, sizeof(UDPLayeredInfo));

	//�޸�Э������ơ���������ΪPFL_HIDDEN��־
	wcscpy(UDPLayeredInfo.szProtocol, wszLSPName);
	UDPLayeredInfo.ProtocolChain.ChainLen = LAYERED_PROTOCOL;
	UDPLayeredInfo.dwProviderFlags |= PFL_HIDDEN;

	//��װ
	if (::WSCInstallProvider(&ProviderGuid, wszDllPath, &UDPLayeredInfo, 1, &nError) == SOCKET_ERROR)
	{
		return nError;
	}

	//����ö��Э���ȡ�ֲ�Э���Ŀ¼ID��
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
	
	//��װЭ�������޸�Э������
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
	//�����ǵķֲ�һЩ���ڴ�Э������Ķ���
	UDPChainInfo.ProtocolChain.ChainEntries[0] = dwLayeredCatalogId;

	//��ȡһ��Guid����װ
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

	//��������WinsockĿ¼�������ǵ�Э������ǰ
	//����ö�ٰ�װЭ��
	FreeProvider(pProtoInfo);
	pProtoInfo = GetProviderw(&nProtocols);
	DWORD dwIds[20];
	int nIndex = 0;

	//������ǵ�Э����
	for (int i = 0; i < nProtocols; i++)
	{
		if ((pProtoInfo[i].ProtocolChain.ChainLen>1)&&(pProtoInfo[i].ProtocolChain.ChainEntries[0]!=dwLayeredCatalogId))
		{
			dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
		}
	}

	//�������Э��
	for (int  i = 0; i < nProtocols; i++)
	{
		if ((pProtoInfo[i].ProtocolChain.ChainLen<=1)||(pProtoInfo[i].ProtocolChain.ChainEntries[0]!=dwLayeredCatalogId))
		{
			dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
		}

	}

	//��������WinsockĿ¼
	nError = ::WSCWriteProviderOrder(dwIds, nIndex);
	FreeProvider(pProtoInfo);
	return nError;


}

//�Ƴ�LSP
//�Ƴ�LSP�ĺ�����WSCDeinstallProvider,ֻҪΪ������Ҫ�Ƴ����ṩ�ߵ�GUID����
//�Ƴ�LSPʱ��Ҫ�ȸ��ݷֲ�Э���GUID���ҵ���Ŀ¼ID�ţ�Ȼ�����Ƴ���Э������������Ƴ��ֲ�Э����ṩ�ߡ�
void RemoveProvider() {
	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	DWORD dwLayeredCatalogId;
	
	//����Guidȡ�÷ֲ�Э���Ŀ¼ID��
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