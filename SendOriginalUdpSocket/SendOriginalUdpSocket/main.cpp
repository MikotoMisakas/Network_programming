#include"head.h"
#include"ClientSock.h"
CInitSock theSock;

void ComputeUdpPseudoHeaderChecksum(
	IPHeader *pIphdr,
	UDPHeader *pUdphdr,
	char* payload,
	int payloadlen
) {
	char buff[1024];
	char* ptr = buff;
	int chksumlen = 0;
	ULONG zero = 0;

	//包含源ip地址和目的ip地址
	memcpy(ptr, &pIphdr->ipSource, sizeof(pIphdr->ipSource));
	ptr += sizeof(pIphdr->ipSource);
	chksumlen += sizeof(pIphdr->ipSource);
	memcpy(ptr, &pIphdr->ipDestination, sizeof(pIphdr->ipDestination));
	ptr += sizeof(pIphdr->ipDestination);
	chksumlen += sizeof(pIphdr->ipDestination);

	//包含8位0域
	memcpy(ptr, &zero, 1);
	ptr += 1;
	chksumlen += 1;

	//协议
	memcpy(ptr, &pIphdr->ipProtocol, sizeof(pIphdr->ipProtocol));
	ptr += sizeof(pIphdr->ipProtocol);
	chksumlen += sizeof(pIphdr->ipProtocol);

	//UDP长度
	memcpy(ptr, &pUdphdr->len, sizeof(pUdphdr->len));
	ptr += sizeof(pUdphdr->len);
	chksumlen += sizeof(pUdphdr->len);

	//UDP源端口号
	memcpy(ptr, &pUdphdr->sourcePort, sizeof(pUdphdr->sourcePort));
	ptr += sizeof(pUdphdr->sourcePort);
	chksumlen += sizeof(pUdphdr->sourcePort);

	//UDP目的端口号
	memcpy(ptr, &pUdphdr->destinationPort, sizeof(pUdphdr->destinationPort));
	ptr += sizeof(pUdphdr->destinationPort);
	chksumlen += sizeof(pUdphdr->destinationPort);

	//又是UDP长度
	memcpy(ptr, &pUdphdr->len, sizeof(pUdphdr->len));
	ptr += sizeof(pUdphdr->len);
	chksumlen += sizeof(pUdphdr->len);

	//16位的UDP校验和，置为0
	memcpy(ptr, &zero, sizeof(USHORT));
	ptr += sizeof(USHORT);
	chksumlen += sizeof(USHORT);

	//净荷
	memcpy(ptr, payload, payloadlen);
	ptr += payloadlen;
	chksumlen += payloadlen;

		//补齐到下一个16位边界
	for (int i = 0; i < payloadlen % 2; i++) {
			*ptr = 0;
			ptr++;
			chksumlen++;
		}
	pUdphdr->checksum = checksum((USHORT*)buff, chksumlen);//计算这个校验和，将结构填充到UDP头
}

int main() {
	//输入参数信息
		char szDestIp[] = "10.18.133.17";//目的ip地址
		char szSourceIp[] = "10.18.133.18";//本机ip地址
		USHORT nDestPort = 4567;
		USHORT nSourcePort = 8888;
		char szMsg[] = "This is a test\r\n";
		int nMsgLen = strlen(szMsg);
		SOCKET sRaw = ::socket(AF_INET, SOCK_RAW, IPPROTO_UDP);//创建原始套接字
		printf("socket dailed%d\n", ::WSAGetLastError());
		//有效IP头包含选项
		BOOL bIncl = TRUE;
		::setsockopt(sRaw, IPPROTO_IP, IP_HDRINCL, (char*)&bIncl, sizeof(bIncl));
		printf("socketopt dailed%d\n", ::WSAGetLastError());
		char buff[1024] = {0};

		//IP头
		IPHeader *pIphdr = (IPHeader*)buff;
		pIphdr->iphVerLen = (4 << 4 | (sizeof(IPHeader) / sizeof(ULONG)));
		pIphdr->ipLength = ::htons(sizeof(IPHeader) + sizeof(UDPHeader) + nMsgLen);
		pIphdr->ipTTL = 128;
		pIphdr->ipProtocol = IPPROTO_UDP;
		pIphdr->ipSource = ::inet_addr(szSourceIp);
		pIphdr->ipDestination = ::inet_addr(szDestIp);
		pIphdr->ipChecksum = checksum((USHORT*)pIphdr, sizeof(IPHeader));

		//UDP头部
		UDPHeader  *pUdphdr = (UDPHeader*)&buff[sizeof(IPHeader)];
		pUdphdr->sourcePort = htons(8888);
		pUdphdr->destinationPort = htons(nDestPort);
		pUdphdr->len = htons(sizeof(IPHeader) + nMsgLen);
		pUdphdr->checksum = 0;
		char* pData = &buff[sizeof(IPHeader) + sizeof(UDPHeader)];
		memcpy(pData, szMsg, nMsgLen);
		ComputeUdpPseudoHeaderChecksum(pIphdr, pUdphdr, pData, nMsgLen);


		//设置目的地址
		SOCKADDR_IN destAddr = { 0 };
		destAddr.sin_family = AF_INET;
		destAddr.sin_port = htons(nDestPort);
		destAddr.sin_addr.S_un.S_addr = ::inet_addr(szDestIp);


		//发送原始UDP封包
		int nRet;
		for (int  i = 0; i <50000; i++)
		{
			nRet = ::sendto(sRaw, buff, sizeof(IPHeader) + sizeof(UDPHeader) + nMsgLen, 0, (sockaddr*)&destAddr, sizeof(destAddr));
			if (nRet==SOCKET_ERROR)
			{
				printf("dendto dailed%d\n", ::WSAGetLastError());
				break;

			}
			else
			{
				printf("sent%d bytes\n", nRet);
			}

		}
	::closesocket(sRaw);
	return 0;
}