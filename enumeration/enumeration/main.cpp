#include"ClientSock.h"
#include"head.h"


CInitSock theSock;



LPWSAPROTOCOL_INFO GetProvider(LPINT lpnTotalProtocols) {
	DWORD dwSize = 0;
	LPWSAPROTOCOL_INFO pProtoInfo = NULL;

	//��ȡ���軺��������
	if (::WSAEnumProtocols(NULL,pProtoInfo,&dwSize)==SOCKET_ERROR)
	{
		if (::WSAGetLastError()!=WSAENOBUFS)
		{
			return NULL;
		}


	}

	//���뻺�����ٴε���
	pProtoInfo = (LPWSAPROTOCOL_INFO)::GlobalAlloc(GPTR, dwSize);
	*lpnTotalProtocols = ::WSAEnumProtocols(NULL, pProtoInfo, &dwSize);
	return pProtoInfo;
}


void FreeProvider(LPWSAPROTOCOL_INFO pProtoInfo) {
	::GlobalFree(pProtoInfo);
}





int mains() {


	int nTotalProtocols;
	LPWSAPROTOCOL_INFO pProtoInfo = GetProviderw(&nTotalProtocols);
	if (pProtoInfo!=NULL)
	{
		//��ӡ����Э���ṩ�ߵ���Ϣ
		for (int i = 0; i < nTotalProtocols; i++)
		{
			printf("Protocol:%s\n", pProtoInfo[i].szProtocol);
			printf("CatalogEntryId:%d	ChainLen:%d\n\n", pProtoInfo[i].dwCatalogEntryId, pProtoInfo[i].ProtocolChain.ChainLen);

		}
		FreeProvider(pProtoInfo);

	}











	return 0;
}

int main() {


	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	pProtoInfo = GetProvider(&nProtocols);
	for (int i = 0; i < nProtocols; i++)
	{
		printf("Protocol:%ws\n", pProtoInfo[i].szProtocol);
		printf("zCatalogEntryId:%d	ChainLen:%d\n\n,", pProtoInfo[i].dwCatalogEntryId, pProtoInfo[i].ProtocolChain.ChainLen);

	}


	return 0;
}