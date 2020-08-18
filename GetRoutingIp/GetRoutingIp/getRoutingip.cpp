#include"head.h"
#include"ClientSock.h"
CInitSock theSock;



int main() {
	const char* szDestIp = "192.168.56.102";
	char recvBuf[1024] = {0};
	//�������ڽ���ICMP�����ԭʼ�׽��֣��󶨵����ض˿�
	SOCKET sRaw = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	sockaddr_in in;
	in.sin_family = AF_INET;
	in.sin_port = 0;
	in.sin_addr.S_un.S_addr = INADDR_ANY;
	if (::bind(sRaw,(sockaddr*)&in,sizeof(in))==SOCKET_ERROR)
	{
		printf("bind() failed\n");
		return 0;
	}
	SetTimeout(sRaw, 5 * 1000);
	//��������UDP������׽���
	SOCKET sSend = ::socket(AF_INET, SOCK_DGRAM, 0);
	SOCKADDR_IN destAddr;
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = ::htons(22);
	destAddr.sin_addr.S_un.S_addr = ::inet_addr(szDestIp);
	int nTTL = 1;
	int nRet;
	ICMP_HDR* pICMPHdr;
	int nTick;
	SOCKADDR_IN recvAddr;
	do
	{
		//����UDP�����TTLֵ
		SetTTL(sSend, nTTL);
		nTick = ::GetTickCount();
		//�������udp���
		nRet = ::sendto(sSend, "hello", 5, 0, (sockaddr*)&destAddr, sizeof(destAddr));
		if (nRet==SOCKET_ERROR)
		{
			printf("dendto failed\n");
			break;
		}
		//�ȴ�����·�������ص�ICMP����
		int nLen = sizeof(recvAddr);
		nRet = ::recvfrom(sRaw, recvBuf, 1024, 0, (sockaddr*)&recvAddr, &nLen);
		if (nRet==SOCKET_ERROR)
		{
			if (::WSAGetLastError()==WSAETIMEDOUT)
			{
				printf("time out\n");
				break;

			}
			else
			{
				printf("recvfrom failed\n");
				break;
			}
		}
		//�����յ���ICMP����
		pICMPHdr = (ICMP_HDR*)&recvBuf[20];
		if (pICMPHdr->icmp_type!=11&&pICMPHdr->icmp_type!=0&&pICMPHdr->icmp_code!=0)
		{
			printf("Unexpected Type:%d, code:%d\n",
				pICMPHdr->icmp_type,pICMPHdr->icmp_code
				);
		}
		char* szIP = ::inet_ntoa(recvAddr.sin_addr);
		printf("��%d��·������IP��ַ%s\n",nTTL, szIP);
		printf("    ��ʱ%d����\n", ::GetTickCount() - nTick);
		if (pICMPHdr->icmp_type==0&&pICMPHdr->icmp_code==0)
		{
			char* sz = &((char*)pICMPHdr)[sizeof(ICMP_HDR)];
			sz[nRet] = '\0';
			printf("Ŀ��ɴ%s\n", sz);
			break;
		}
		printf("--------------------------------");
	} while (nTTL++<=20);
	::closesocket(sRaw);
	::closesocket(sSend);
	return 0;
}
