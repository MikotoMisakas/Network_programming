//Ӧ�ó�����Ե���ioctlsocket������ʽ���׽��ֹ����ڷ�����ģʽ
/*
u_long ul=1;
SOCKET s=socket(AF_INFO,SOCKET_STREAM,0);
ioctlsocket(s,FIONBIO,(u_long*)&ul);
*/

//1����ʼ���׽��ּ���fdSocket�������������Ӽ����׽��־��
//2����fdSocket���ϵĿ���fdRead���ݸ�select�����������¼�����ʱ��select������fdRead������û��δ��I/O�������׽��־����Ȼ�󷵻�
//3���Ƚ�ԭ��fdSocket������socket�������fdRead���ϣ�ȷ����Щ�׽�����δ��I/O����һ��������ЩI/O
//4.���ص��ڶ�����������ѡ����
#include"head.h"
#include"ClientSock.h"
CInitSock theSock;//��ʼ��Winsock
int main() {

	USHORT nPort = 4567;

	//���������׽���
	SOCKET sListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(nPort);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;

	//���׽��ֵ����ػ���
	if (::bind(sListen,(sockaddr*)&sin,sizeof(sin))==SOCKET_ERROR)
	{
		printf("Failed bind()\n");
		return -1;
	}

	//�������ģʽ
	::listen(sListen, 5);

	//select ģ�ʹ������
	//1����ʼ��һ���׽��ּ���fdSocket������׽��־�����������
	fd_set fdSocket;//���п����׽��ּ���
	FD_ZERO(&fdSocket);
	FD_SET(sListen, &fdSocket);
	while (TRUE)
	{
		//2��fdSocket���ϵ�һ������fdRead���ݸ�select����
		//�����¼�����ʱ��select�����Ƴ�fdRead������û��δ��IO�������׽��־����Ȼ�󷵻�
		fd_set fdRead = fdSocket;
		int nRet = ::select(0, &fdRead, NULL, NULL, NULL);
		if (nRet > 0) {
			//3��ͨ����ԭ��fdSocket������select�������fdRead���ϱȽ�
			//ȷ������Щ�׽�����δ��IO������һ��������ЩIO
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
							printf("���յ����ӣ�%s��\n", ::inet_ntoa(addrRemote.sin_addr));

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
						if (nRecv>0)//�ɶ�
						{
							szText[nRecv] = '\0';
							printf("���յ����ݣ�%s\n", szText);
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
select�ĺô������ڵ����߳��ڴ������׽������ӣ�����������ģʽ���߳����͵����⣬������ӵ�fd_set�ṹ���׽������������޵ģ�Ĭ������£����ֵ��FD_SETSIZE����winsock2.h�ļ��ж���Ϊ64��Ϊ��
�����׽������������Խ�FD_SETSIZE����Ϊ�����ֵ��������������winsock2.h֮ǰ���������Զ����ֵҲ���ܳ���Winsock�²��ṩ�ߣ�ͨ����1024


FD_SETSIZEֵ̫��Ļ������������ܾͻ���Ӱ�죬������1000���׽��֣���ô�ڵ���select֮ǰ���ò�������1000���׽��֣�select����֮���ֱ�������1000���׽���


*/