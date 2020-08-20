#include"ClientSock.h"
#include"head.h"
#include"string.h"
CInitSock theSock;

#define SOURCE_PORT 7234
#define MAX_RECEIVEVYTE 255



int main(int argc,char* argv[]) {

	WSADATA WSAData;
	SOCKET SOCK;
	SOCKADDR_IN addr_in;
	IP_HEADER ipHeader;
	TCP_HEADER tcpHeader;
	PSD_HEADER psdHeader;
	char szSendBuf[60] = { 0 };
	BOOL flag;
	int rect, nTimeOver;
	useage();

	if (WSAStartup(MAKEWORD(2,2),&WSAData)!=0)
	{
		printf("WSAStartup Error!\n");
		return false;
	}
	if ((SOCK=WSASocket(AF_INET,SOCK_RAW,IPPROTO_RAW,NULL,0,WSA_FLAG_OVERLAPPED))==INVALID_SOCKET)
	{
		printf("Socket Setup Error！\n");
		return false;
	}
	flag = true;
	if (setsockopt(SOCK,IPPROTO_IP,IP_HDRINCL,(char*)&flag,sizeof(flag))==SOCKET_ERROR)
	{
		printf("setsockopt IP_HDRINCL error\n");
		return false;

	}
	nTimeOver = 1000;
	if (setsockopt(SOCK,SOL_SOCKET,SO_SNDTIMEO,(char*)&nTimeOver,sizeof(nTimeOver))==SOCKET_ERROR)
	{
		printf("SETSOCKOPT so_sndtimeo error");
		return false;
	}

	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(atoi("5678"));
	addr_in.sin_addr.S_un.S_addr = inet_addr("10.18.133.17");

	//填充IP首部
	ipHeader.h_lenver = (4 << 4 | sizeof(ipHeader) / sizeof(unsigned long));
	//ipHeader.tos = 0;
	ipHeader.total_len = htons(sizeof(ipHeader) + sizeof(tcpHeader));
	ipHeader.ident = 1;
	ipHeader.frag_and_flags = 0;
	ipHeader.ttl = 128;
	ipHeader.proto = IPPROTO_TCP;
	ipHeader.checksum = 0;
	ipHeader.sourceIP = inet_addr("10.18.133.18");
	ipHeader.destIP = inet_addr("10.18.133.17");

	//填充TCP首部
	tcpHeader.th_dport = htons(atoi("5678"));
	tcpHeader.th_sport = htons(SOURCE_PORT);//源端口号
	tcpHeader.th_seq = htonl(0x12345678);
	tcpHeader.th_ack = 0;
	tcpHeader.th_lenres = (sizeof(tcpHeader) / 4 << 4 | 0);
	tcpHeader.th_flag = 2;//修改这里实现不同的标志为探测2为syn 1是fin 16是ack
	tcpHeader.th_win = htons(512);
	tcpHeader.th_urp = 0;
	tcpHeader.th_sum = 0;
	psdHeader.saddr = ipHeader.sourceIP;
	psdHeader.daddr = ipHeader.destIP;
	psdHeader.mbz = 0;
	psdHeader.ptcl = IPPROTO_TCP;
	psdHeader.tcpl = htons(sizeof(tcpHeader));


	//计算校验和
	char* s = 0;
	memcpy(szSendBuf, &psdHeader, sizeof(psdHeader));
	memcpy(szSendBuf + sizeof(psdHeader), &tcpHeader, sizeof(tcpHeader));
	tcpHeader.th_sum = checksum((USHORT*)szSendBuf, sizeof(psdHeader) + sizeof(tcpHeader));
	memcpy(szSendBuf, &ipHeader, sizeof(ipHeader));
	memcpy(szSendBuf + sizeof(ipHeader), &tcpHeader, sizeof(tcpHeader));
	szSendBuf[sizeof(ipHeader) + sizeof(tcpHeader)];
	memset(szSendBuf + sizeof(ipHeader) + sizeof(tcpHeader), 0, 4);
	ipHeader.checksum = checksum((USHORT*)szSendBuf, sizeof(ipHeader) + sizeof(tcpHeader));
	memcpy(szSendBuf, &ipHeader, sizeof(ipHeader));
	rect = sendto(SOCK, szSendBuf, sizeof(ipHeader) + sizeof(tcpHeader),0,(struct sockaddr*)&addr_in,sizeof(addr_in));
	if (rect==SOCKET_ERROR)
	{
		printf("SEND ERROR!:/d\N,", WSAGetLastError());
		return false;

	}
	else
	{
		printf("send ok");
	}
	closesocket(SOCK);
	WSACleanup();
	




	return 0;
}
/*
和发送原始套接字相比，接收就比较麻烦了，因为在win不能使用recv接收raw sock上的数据，这是因为所有的ip包都是先递交给系统核心，然后在传输到用户程序。当发送一个raws socket包的时候
核心并不知道，也没有这个数据被发送或连接建立的记录。因此当远程主机回应的时候，系统就把这些包全部丢弃了。从而到不了应用程序，所以不能使用简单的接收函数来接收这些数据报

要达到接收数据的目的，必须采用嗅探，接收所有通过的数据包，然后筛选，留下符合我们需要的。再定义一个原始套接字，完成接收数据的任务，需要设置SIO_RCVALL，标识接收所有数据
SOCKET sniffersock;
sniffersock = WSASocket(AF_INET, SOCK_RAW, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
DWORD lpvBuffer = 1;
DWORD LPCBbYTESrETURNED = 0;
WSAIoctl(sniffersock, SIO_RCVALL, &lpvBuffer, sizeof(lpvBuffer), NULL, 0, &lpcbBytesReturned, NULL, NULL);
*/
