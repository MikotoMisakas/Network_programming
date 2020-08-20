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
		printf("Socket Setup Error��\n");
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

	//���IP�ײ�
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

	//���TCP�ײ�
	tcpHeader.th_dport = htons(atoi("5678"));
	tcpHeader.th_sport = htons(SOURCE_PORT);//Դ�˿ں�
	tcpHeader.th_seq = htonl(0x12345678);
	tcpHeader.th_ack = 0;
	tcpHeader.th_lenres = (sizeof(tcpHeader) / 4 << 4 | 0);
	tcpHeader.th_flag = 2;//�޸�����ʵ�ֲ�ͬ�ı�־Ϊ̽��2Ϊsyn 1��fin 16��ack
	tcpHeader.th_win = htons(512);
	tcpHeader.th_urp = 0;
	tcpHeader.th_sum = 0;
	psdHeader.saddr = ipHeader.sourceIP;
	psdHeader.daddr = ipHeader.destIP;
	psdHeader.mbz = 0;
	psdHeader.ptcl = IPPROTO_TCP;
	psdHeader.tcpl = htons(sizeof(tcpHeader));


	//����У���
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
�ͷ���ԭʼ�׽�����ȣ����վͱȽ��鷳�ˣ���Ϊ��win����ʹ��recv����raw sock�ϵ����ݣ�������Ϊ���е�ip�������ȵݽ���ϵͳ���ģ�Ȼ���ڴ��䵽�û����򡣵�����һ��raws socket����ʱ��
���Ĳ���֪����Ҳû��������ݱ����ͻ����ӽ����ļ�¼����˵�Զ��������Ӧ��ʱ��ϵͳ�Ͱ���Щ��ȫ�������ˡ��Ӷ�������Ӧ�ó������Բ���ʹ�ü򵥵Ľ��պ�����������Щ���ݱ�

Ҫ�ﵽ�������ݵ�Ŀ�ģ����������̽����������ͨ�������ݰ���Ȼ��ɸѡ�����·���������Ҫ�ġ��ٶ���һ��ԭʼ�׽��֣���ɽ������ݵ�������Ҫ����SIO_RCVALL����ʶ������������
SOCKET sniffersock;
sniffersock = WSASocket(AF_INET, SOCK_RAW, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
DWORD lpvBuffer = 1;
DWORD LPCBbYTESrETURNED = 0;
WSAIoctl(sniffersock, SIO_RCVALL, &lpvBuffer, sizeof(lpvBuffer), NULL, 0, &lpcbBytesReturned, NULL, NULL);
*/
