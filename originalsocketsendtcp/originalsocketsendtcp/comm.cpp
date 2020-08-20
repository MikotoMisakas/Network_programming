//////////////////////////////////////////////////
// comm.cpp�ļ�

#include"head.h"

#include "comm.h"

//��Ҫ�Լ����ipͷ��TCPͷ��ʱ�򣬾���Ҫ�Լ��������ǵ�У���
USHORT checksum(USHORT* buff, int size)
{
	unsigned long cksum = 0;
	while (size > 1)
	{
		cksum += *buff++;
		size -= sizeof(USHORT);
	}
	// ������
	if (size)
	{
		cksum += *(UCHAR*)buff;
	}
	// ��32λ��chsum��16λ�͵�16λ��ӣ�Ȼ��ȡ��
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);			// ???	
	return (USHORT)(~cksum);
}

BOOL SetTTL(SOCKET s, int nValue)
{
	int ret = ::setsockopt(s, IPPROTO_IP, IP_TTL, (char*)&nValue, sizeof(nValue));
	return ret != SOCKET_ERROR;
}

BOOL SetTimeout(SOCKET s, int nTime, BOOL bRecv)
{
	int ret = ::setsockopt(s, SOL_SOCKET,
		bRecv ? SO_RCVTIMEO : SO_SNDTIMEO, (char*)&nTime, sizeof(nTime));
	return ret != SOCKET_ERROR;
}

void useage() {
	printf("*****************************\n");
	printf("\tWritten by Refdom\n");
	printf("\tEmail:\n");
	printf("Useage tcp\n");
	printf("******************************\n");
}
