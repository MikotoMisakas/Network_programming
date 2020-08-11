#include"ClientSock.h"
#include"head.h"
CInitSock initsoc;


int main() {

	//	创建套接字
	SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {


		printf("Failed socket()\n");
		return 0;
	}

	//填写远程地址信息
	sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(4567);

	//填写服务器程序所在机器的ip地址
	//本地地址可以直接填写127.0.0.1

	servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (::connect(s, (sockaddr*)&servAddr, sizeof(servAddr)) == -1)

	{
		printf("failed connect()\n");
		return 0;
	}
	char szText[] = "TCP Service Demo!\r\n";
	::send(s, szText, strlen(szText), 0);

	//接收数据
	//char buff[256];
	//int nRecv = ::recv(s, buff, 256, 0);
	//if (nRecv > 0)
	//{
	//	buff[nRecv] = '\0';
	//	printf("收到数据%s", buff);
	//}

	//关闭套接字
	::closesocket(s);



	return 0;
}