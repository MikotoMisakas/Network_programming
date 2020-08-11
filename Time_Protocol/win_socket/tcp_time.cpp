#include"head.h"
#include"ClientSock.h"
CInitSock initSocket;

void SetTimeFromTP(ULONG ulTime)//根据时间协议返回的时间设置系统时间
{
	//windows文件时间是一个64位的值，他从1601年1月1日中午十二点到现在的时间间隔
	//单位是1/1000 0000秒，1000万分之一秒
	FILETIME ft;
	SYSTEMTIME st;
	st.wYear = 1900;
	st.wMonth = 1;
	st.wDay = 1;
	st.wHour = 0;
	st.wMinute = 0;
	st.wSecond = 0;
	st.wMilliseconds = 0;
	SystemTimeToFileTime(&st, &ft);
	//然后使用Time Protocol使用基准时间加上逝去时间，既ulTime
	LONGLONG *pLLong = (LONGLONG*)&ft;

	//注意文件的时间单位值千万分之一秒
	*pLLong += (LONGLONG)10000000 * ulTime;

	//在将时间转换回来，更新系统时间
	FileTimeToSystemTime(&ft, &st);
	SetSystemTime(&st);



}
 
int main() {

	SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		printf("Failed socket()\n");
		return 0;
	}


	//填写远程地址信息，连接到时间服务器
	sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(123);

	servAddr.sin_addr.S_un.S_addr = inet_addr("202.120.2.101");
	if (::connect(s, (sockaddr*)&servAddr, sizeof(servAddr)) == -1)
	{
		printf("Failed connect()\n");
		return 0;

	}

	//等待接收时间协议返回的时间，学习了windows I/O模型之后，最好使用异步I/O，以便设置超时
	ULONG ulTime = 0;
	int nRecv = ::recv(s, (char*)&ulTime, sizeof(ulTime), 0);
	if (nRecv > 0)
	{
		ulTime = ntohl(ulTime);
		SetTimeFromTP(ulTime);
		printf("成功与时间服务器的时间同步\n");

	}
	else
	{
		printf("时间服务器不能确定当前时间！\n");
	}
	::closesocket(s);
	return 0;
}