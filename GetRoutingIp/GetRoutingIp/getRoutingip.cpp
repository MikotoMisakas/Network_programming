#include"head.h"
#include"ClientSock.h"
CInitSock theSock;



int main() {
	const char* szDestIp = "192.168.56.102";
	char recvBuf[1024] = {0};
	//创建用于接收ICMP封包的原始套接字，绑定到本地端口
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
	//创建用于UDP封包的套接字
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
		//设置UDP封包的TTL值
		SetTTL(sSend, nTTL);
		nTick = ::GetTickCount();
		//发送这个udp封包
		nRet = ::sendto(sSend, "hello", 5, 0, (sockaddr*)&destAddr, sizeof(destAddr));
		if (nRet==SOCKET_ERROR)
		{
			printf("dendto failed\n");
			break;
		}
		//等待接收路由器返回的ICMP报文
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
		//解析收到的ICMP数据
		pICMPHdr = (ICMP_HDR*)&recvBuf[20];
		if (pICMPHdr->icmp_type!=11&&pICMPHdr->icmp_type!=0&&pICMPHdr->icmp_code!=0)
		{
			printf("Unexpected Type:%d, code:%d\n",
				pICMPHdr->icmp_type,pICMPHdr->icmp_code
				);
		}
		char* szIP = ::inet_ntoa(recvAddr.sin_addr);
		printf("第%d个路由器，IP地址%s\n",nTTL, szIP);
		printf("    用时%d毫秒\n", ::GetTickCount() - nTick);
		if (pICMPHdr->icmp_type==0&&pICMPHdr->icmp_code==0)
		{
			char* sz = &((char*)pICMPHdr)[sizeof(ICMP_HDR)];
			sz[nRet] = '\0';
			printf("目标可达：%s\n", sz);
			break;
		}
		printf("--------------------------------");
	} while (nTTL++<=20);
	::closesocket(sRaw);
	::closesocket(sSend);
	return 0;
}
