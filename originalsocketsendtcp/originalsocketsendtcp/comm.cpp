//////////////////////////////////////////////////
// comm.cpp文件

#include"head.h"

#include "comm.h"

//需要自己填充ip头和TCP头的时候，就需要自己计算他们的校验和
USHORT checksum(USHORT* buff, int size)
{
	unsigned long cksum = 0;
	while (size > 1)
	{
		cksum += *buff++;
		size -= sizeof(USHORT);
	}
	// 是奇数
	if (size)
	{
		cksum += *(UCHAR*)buff;
	}
	// 将32位的chsum高16位和低16位相加，然后取反
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
