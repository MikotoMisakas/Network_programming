#include"head.h"
#include"ClientSock.h"
CInitSock theSock;




void DecodeTCPPacket(char * pData) {
	TCPHeader* pTCPHdr = (TCPHeader*)pData;
	printf("port:%d->%d\n", ntohs(pTCPHdr->sourcePort), ntohs(pTCPHdr->destinationPort));
	//�����Ը���Ŀ�Ķ˿ںŽ�һ������Ӧ�ò�Э��
	switch (::ntohs(pTCPHdr->destinationPort))
	{
	case 21:
		break;
	case 80:
	case 8080:
		break;
	}
	return;
}

void DecodeIPPacket(char* pData) {
	IPHeader *pIPHdr = (IPHeader*)pData;
	in_addr source, dest;
	char szSourceIP[32], szDestIP[32];
	printf("--------------------------------");
	//��ipͷȡ��ip��ַ��Ŀ�ĵ�ַ
	source.S_un.S_addr = pIPHdr->ipSource;
	dest.S_un.S_addr = pIPHdr->ipDestination;
	strcpy(szSourceIP, ::inet_ntoa(source));
	strcpy(szDestIP, ::inet_ntoa(dest));
	printf("%s----->%s\n", szSourceIP, szDestIP);

	//IPͷ����
	int nHeaderLen = (pIPHdr->iphVerLen & 0xf) * sizeof(ULONG);
	switch (pIPHdr->ipProtocol)
	{
	case IPPROTO_TCP://TCPЭ��
		DecodeTCPPacket(pData + nHeaderLen);
		break;
	case IPPROTO_UDP://TCPЭ��
		printf("udp");
		break;
	case IPPROTO_ICMP://TCPЭ��
		printf("tcp");
		break;
	}
}







int main() {
	//����ԭʼ�׽���
	SOCKET sRaw = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	//��ȡ����IP��ַ
	char szHostName[56];
	SOCKADDR_IN addr_in;
	struct hostent* pHost;
	gethostname(szHostName, 56);
	if ((pHost = gethostbyname((char*)szHostName)) == NULL)
		return 0;
	//����ioctl֮ǰ�׽��ֱ����
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(0);
	printf("%s,",pHost->h_addr_list[1]);
	memcpy(&addr_in.sin_addr.S_un.S_addr, pHost->h_addr_list[1], pHost->h_length);

	printf("Binding to interface :%s\n", ::inet_ntoa(addr_in.sin_addr));
	if (bind(sRaw,(PSOCKADDR)&addr_in,sizeof(addr_in))==SOCKET_ERROR)
	{
		return 0;

	}

	//����SIO_RCVALL�����룬�Ա�������е�ip��
	DWORD dwValue = 1;
	if (ioctlsocket(sRaw,SIO_RCVALL,&dwValue)!=0)
	{
		return 0;

	}
	
	//��ʼ���շ��
	char buff[1024];
	int nRet;
	while (TRUE)
	{
		nRet = recv(sRaw, buff, 1024, 0);
		if (nRet>0)
		{
			DecodeIPPacket(buff);

		}
	}
	closesocket(sRaw);
	return 0;
}