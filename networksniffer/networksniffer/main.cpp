#include"head.h"
#include"ClientSock.h"
CInitSock theSock;




void DecodeTCPPacket(char * pData) {
	TCPHeader* pTCPHdr = (TCPHeader*)pData;
	printf("port:%d->%d\n", ntohs(pTCPHdr->sourcePort), ntohs(pTCPHdr->destinationPort));
	//还可以根据目的端口号进一步解析应用层协议
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
	//从ip头取出ip地址和目的地址
	source.S_un.S_addr = pIPHdr->ipSource;
	dest.S_un.S_addr = pIPHdr->ipDestination;
	strcpy(szSourceIP, ::inet_ntoa(source));
	strcpy(szDestIP, ::inet_ntoa(dest));
	printf("%s----->%s\n", szSourceIP, szDestIP);

	//IP头长度
	int nHeaderLen = (pIPHdr->iphVerLen & 0xf) * sizeof(ULONG);
	switch (pIPHdr->ipProtocol)
	{
	case IPPROTO_TCP://TCP协议
		DecodeTCPPacket(pData + nHeaderLen);
		break;
	case IPPROTO_UDP://TCP协议
		printf("udp");
		break;
	case IPPROTO_ICMP://TCP协议
		printf("tcp");
		break;
	}
}







int main() {
	//创建原始套接字
	SOCKET sRaw = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	//获取本地IP地址
	char szHostName[56];
	SOCKADDR_IN addr_in;
	struct hostent* pHost;
	gethostname(szHostName, 56);
	if ((pHost = gethostbyname((char*)szHostName)) == NULL)
		return 0;
	//调用ioctl之前套接字必须绑定
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(0);
	printf("%s,",pHost->h_addr_list[1]);
	memcpy(&addr_in.sin_addr.S_un.S_addr, pHost->h_addr_list[1], pHost->h_length);

	printf("Binding to interface :%s\n", ::inet_ntoa(addr_in.sin_addr));
	if (bind(sRaw,(PSOCKADDR)&addr_in,sizeof(addr_in))==SOCKET_ERROR)
	{
		return 0;

	}

	//设置SIO_RCVALL控制码，以便接收所有的ip包
	DWORD dwValue = 1;
	if (ioctlsocket(sRaw,SIO_RCVALL,&dwValue)!=0)
	{
		return 0;

	}
	
	//开始接收封包
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