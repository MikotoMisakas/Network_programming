#include"ClientSock.h"
#include"head.h"
CInitSock theSock;



typedef struct _icmp_hdr {
	unsigned char icmp_type;//消息类型
	unsigned char icmp_code;//代码
	unsigned short icmp_checksum;//校验和
	//下面是回显头
	unsigned icmp_id;//用来唯一标识此请求的ID号，通常设置为进程ID
	unsigned icmp_sequence;//序列号
	unsigned icmp_timestamp;//事件戳
}ICMP_HDR,*PICMP_HDR;


int main() {

	char szDestIp[] = "10.18.133.17";
	SOCKET sRaw = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);//创建原始套接字

	SetTimeout(sRaw, 1000, TRUE);//设置接收超时
	SOCKADDR_IN dest;
	dest.sin_family = AF_INET;
	dest.sin_port = htons(0);
	dest.sin_addr.S_un.S_addr = inet_addr(szDestIp);

	//创建ICMP封包
	char buff[sizeof(ICMP_HDR) + 32];
	ICMP_HDR* pIcmp = (ICMP_HDR*)buff;


	//天下ICMP封包数据
	pIcmp->icmp_type = 8;//请求一个ICMP回显
	pIcmp->icmp_code = 0;
	pIcmp->icmp_id = (USHORT)::GetCurrentProcessId();
	pIcmp->icmp_checksum = 0;
	pIcmp->icmp_sequence = 0;

	//填充数据部分可以为任意
	memset(&buff[sizeof(ICMP_HDR)], 'E', 32);


	//开始发送和接收ICMP封包
	USHORT nSeq = 0;
	char recvBuf[1024];
	SOCKADDR_IN from;
	int nLen = sizeof(from);
	while (TRUE)
	{
		static int nCount = 0;
		int nRet;
		if (nCount++==4)
		{
			break;
		}
		pIcmp->icmp_checksum = 0;
		pIcmp->icmp_timestamp = ::GetTickCount();
		pIcmp->icmp_sequence = nSeq++;
		pIcmp->icmp_checksum = checksum((USHORT*)buff, sizeof(ICMP_HDR) + 32);
		nRet = ::sendto(sRaw, buff, sizeof(ICMP_HDR) + 32, 0, (SOCKADDR*)&dest, sizeof(dest));
		if (nRet==SOCKET_ERROR)
		{
			printf("sendto failed:%d\n", ::WSAGetLastError());
			return 0;
		}
		nRet = ::recvfrom(sRaw, recvBuf, 1024, 0, (sockaddr*)&from, &nLen);
		if (nRet==SOCKET_ERROR)
		{
			if (::WSAGetLastError()==WSAETIMEDOUT)
			{
				printf("timed out\n");
				continue;
			}
			printf("recvfrom failed%d\n", ::WSAGetLastError());
			return -1;
		}

		//解析收到的ICMP封包
		int nTick = ::GetTickCount();
		if (nRet<sizeof(IPHeader)+sizeof(ICMP_HDR))
		{
			printf("too few bytes from %s\n", ::inet_ntoa(from.sin_addr));
		}
		ICMP_HDR *pPecvIcmp = (ICMP_HDR*)(recvBuf + sizeof(IPHeader));
		if (pPecvIcmp->icmp_type!=0)//回显
		{
			printf("nonecho type%d recvd \n", pPecvIcmp->icmp_type);
			return -1;
		}
		if (pPecvIcmp->icmp_id!=GetCurrentProcessId())
		{
			printf("someone else packet!\n");
			return -1;
		}
		 
		printf("%d bytes from %s:", nRet, inet_ntoa(from.sin_addr));
		printf("icmp_seq=%d", pPecvIcmp->icmp_sequence);
		printf("time:%d ms", pPecvIcmp->icmp_timestamp);
		printf("\n");
		::Sleep(1000);
	}
	return 0;
}