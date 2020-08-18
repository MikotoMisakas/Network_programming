#include"ClientSock.h"
#include"head.h"
CInitSock theSock;



typedef struct _icmp_hdr {
	unsigned char icmp_type;//��Ϣ����
	unsigned char icmp_code;//����
	unsigned short icmp_checksum;//У���
	//�����ǻ���ͷ
	unsigned icmp_id;//����Ψһ��ʶ�������ID�ţ�ͨ������Ϊ����ID
	unsigned icmp_sequence;//���к�
	unsigned icmp_timestamp;//�¼���
}ICMP_HDR,*PICMP_HDR;


int main() {

	char szDestIp[] = "10.18.133.17";
	SOCKET sRaw = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);//����ԭʼ�׽���

	SetTimeout(sRaw, 1000, TRUE);//���ý��ճ�ʱ
	SOCKADDR_IN dest;
	dest.sin_family = AF_INET;
	dest.sin_port = htons(0);
	dest.sin_addr.S_un.S_addr = inet_addr(szDestIp);

	//����ICMP���
	char buff[sizeof(ICMP_HDR) + 32];
	ICMP_HDR* pIcmp = (ICMP_HDR*)buff;


	//����ICMP�������
	pIcmp->icmp_type = 8;//����һ��ICMP����
	pIcmp->icmp_code = 0;
	pIcmp->icmp_id = (USHORT)::GetCurrentProcessId();
	pIcmp->icmp_checksum = 0;
	pIcmp->icmp_sequence = 0;

	//������ݲ��ֿ���Ϊ����
	memset(&buff[sizeof(ICMP_HDR)], 'E', 32);


	//��ʼ���ͺͽ���ICMP���
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

		//�����յ���ICMP���
		int nTick = ::GetTickCount();
		if (nRet<sizeof(IPHeader)+sizeof(ICMP_HDR))
		{
			printf("too few bytes from %s\n", ::inet_ntoa(from.sin_addr));
		}
		ICMP_HDR *pPecvIcmp = (ICMP_HDR*)(recvBuf + sizeof(IPHeader));
		if (pPecvIcmp->icmp_type!=0)//����
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