
#include"ClientSock.h"
CInitSock initsock;



int main() {

//创建套接字
	SOCKET sListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen==INVALID_SOCKET)
	{
		printf(

			"Failed socket()\n"
		);
		return 0;

	}

	//填充sockaddr_in结构
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(4567);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;

	//绑定这个套接字到本地地址
	if (::bind(sListen,(LPSOCKADDR)&sin,sizeof(sin))==SOCKET_ERROR)
	{
		printf("Failed bind()\n");
		return 0;
	}

	//进入监听模式
	if (::listen(sListen, 2) == SOCKET_ERROR) {


		printf("Failed listen()\n");
		return 0;

	}

	

	//循环接受客户的连接请求
	sockaddr_in remoteAddr;
	int nAddrLen = sizeof(remoteAddr);
	SOCKET sClient;
	char szText[] = "TCP Service Demo!\r\n";
	while(TRUE) {

		//接受新链接
		sClient = ::accept(sListen, (SOCKADDR*)&remoteAddr, &nAddrLen);
		if (sClient==INVALID_SOCKET)
		{
			printf("Failed accept()\n");
			continue;

		}
		printf("接收到一个连接：%s\r\n", inet_ntoa(remoteAddr.sin_addr));

		//向客户端发送数据
		::send(sClient,szText,strlen(szText),0);

		//关闭同客户端的连接
		::closesocket(sListen);

	}

	//关闭套接字
	::closesocket(sListen);
	return 0;



}