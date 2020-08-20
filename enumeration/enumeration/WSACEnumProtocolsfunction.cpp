#include"head.h"

LPWSAPROTOCOL_INFOW GetProviderw(LPINT lpnTotalProtocols) {
	int nError;
	DWORD dwSize = 0;
	LPWSAPROTOCOL_INFOW pProtoInfo = NULL;

	//取得所需的缓冲区长度
	if (::WSCEnumProtocols(NULL,pProtoInfo,&dwSize,&nError)==SOCKET_ERROR)
	{
		if (nError!=WSAENOBUFS)
		{
			return NULL;
		}



	}
	//申请缓冲区，再次调用WSCEnumProcols函数
	pProtoInfo = (LPWSAPROTOCOL_INFOW)::GlobalAlloc(GPTR, dwSize);
	*lpnTotalProtocols = ::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError);
	return pProtoInfo;


}
