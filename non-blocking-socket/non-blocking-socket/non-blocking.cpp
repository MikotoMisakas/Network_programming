//应用程序可以调用ioctlsocket函数显式让套接字工作在非阻塞模式
/*
u_long ul=1;
SOCKET s=socket(AF_INFO,SOCKET_STREAM,0);
ioctlsocket(s,FIONBIO,(u_long*)&ul);
*/

//1、初始化套接字集合fdSocket，向这个集合添加监听套接字句柄
//2、将fdSocket集合的拷贝fdRead传递给select函数，当有事件发生时，select函数除fdRead集合中没有未决I/O操作的套接字句柄，然后返回
//3、比较原来fdSocket集合与socket处理过的fdRead集合，确定哪些套接字有未决I/O并进一步处理这些I/O
//4.、回到第二步继续进行选择处理
#include"head.h"
#include"ClientSock.h"
CInitSock theSock;//初始化Winsock
int main() {

	USHORT nPort = 4567;

	//创建监听套接字
	SOCKET sListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(nPort);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;

	//绑定套接字到本地机器
	if (::bind(sListen,(sockaddr*)&sin,sizeof(sin))==SOCKET_ERROR)
	{
		printf("Failed bind()\n");
		return -1;
	}

	//进入监听模式
	::listen(sListen, 5);

	//select 模型处理过程
	//1、初始化一个套接字集合fdSocket，添加套接字句柄到这个集合
	fd_set fdSocket;//所有可用套接字集合
	FD_ZERO(&fdSocket);
	FD_SET(sListen, &fdSocket);
	while (TRUE)
	{
		//2将fdSocket集合的一个拷贝fdRead传递给select函数
		//当有事件发生时，select函数移除fdRead集合中没有未决IO操作的套接字句柄，然后返回
		fd_set fdRead = fdSocket;
		int nRet = ::select(0, &fdRead, NULL, NULL, NULL);
		if (nRet > 0) {
			//3、通过将原来fdSocket集合与select处理过的fdRead集合比较
			//确定有哪些套接字是未决IO，并进一步处理这些IO
			for (size_t i = 0; i < (int)fdSocket.fd_count; i++)
			{
				if (FD_ISSET(fdSocket.fd_array[i],&fdRead))
				{
					if (fdSocket.fd_array[i]==sListen) {
						if (fdSocket.fd_count<FD_SETSIZE)
						{
							sockaddr_in addrRemote;
							int nAddrLen = sizeof(addrRemote);
							SOCKET SnEW = ::accept(sListen, (SOCKADDR*)&addrRemote, &nAddrLen);
							FD_SET(SnEW, &fdSocket);
							printf("接收到连接（%s）\n", ::inet_ntoa(addrRemote.sin_addr));

						}
						else
						{
							printf("Too much connections!\n");
							continue;
						}
					}
					else
					{
						char szText[256];
						int nRecv = ::recv(fdSocket.fd_array[i], szText, strlen(szText), 0);
						if (nRecv>0)//可读
						{
							szText[nRecv] = '\0';
							printf("接收到数据：%s\n", szText);
						}
						else
						{
							::closesocket(fdSocket.fd_array[i]);
							FD_CLR(fdSocket.fd_array[i], &fdSocket);
						}
					}
				}
			}
		}
		else
		{
			printf("Failed select()\n");
			break;
		}

	}
	return 0;
}

/*
select的好处是能在单个线程内处理多个套接字连接，避免了阻塞模式下线程膨胀的问题，但是添加到fd_set结构的套接字数量是有限的，默认情况下，最大值是FD_SETSIZE，在winsock2.h文件中定义为64，为了
增加套接字数量，可以将FD_SETSIZE定义为更大的值（这个定义必须在winsock2.h之前）不过，自定义的值也不能超过Winsock下层提供者，通常是1024


FD_SETSIZE值太大的话，服务器性能就会受影响，例如有1000个套接字，那么在调用select之前不得不设置这1000个套接字，select返回之后，又必须检查者1000个套接字


*/