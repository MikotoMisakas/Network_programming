#include"head.h"

LPWSAPROTOCOL_INFOW GetProviderw(LPINT lpnTotalProtocols) {
	int nError;
	DWORD dwSize = 0;
	LPWSAPROTOCOL_INFOW pProtoInfo = NULL;

	//ȡ������Ļ���������
	if (::WSCEnumProtocols(NULL,pProtoInfo,&dwSize,&nError)==SOCKET_ERROR)
	{
		if (nError!=WSAENOBUFS)
		{
			return NULL;
		}



	}
	//���뻺�������ٴε���WSCEnumProcols����
	pProtoInfo = (LPWSAPROTOCOL_INFOW)::GlobalAlloc(GPTR, dwSize);
	*lpnTotalProtocols = ::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError);
	return pProtoInfo;


}
