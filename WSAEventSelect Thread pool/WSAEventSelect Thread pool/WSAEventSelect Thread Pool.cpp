#include"ClientSock.h"
#include"head.h"
CInitSock TheSock;


typedef struct _SOCKET_OBJ {//用来记录客户端套接字的信息
	SOCKET s;//套接字句柄
	HANDLE event;//与此套接字相关联的事件对象句柄
	sockaddr_in addrRemote;//客户端地址信息
	_SOCKET_OBJ *pNext;//指向下一个SOCKET_OBJ对象，连成一个表
}SOCKET_OBJ, *PSOCKET_OBJ;

typedef struct _THREAD_OBJ {//用来记录每个线程的信息

	HANDLE events[WSA_MAXIMUM_WAIT_EVENTS];//记录当前要等待记录事件对象的句柄
	int nSocketCount;//记录当前线程处理套接字数量《=WSA_MAXIMUM_WAIT_EVENTS
	PSOCKET_OBJ pSockHeader;//当前线程处理套接字对象列表，pSockHeader指向表头
	PSOCKET_OBJ pSockTail;//pSockTail指向表尾
	CRITICAL_SECTION cs;//关键代码段变量，位的是同步对本结构的访问
	_THREAD_OBJ* pNext;//指向下一个THREAD_OBJ对象，为了连接为一个表

}THREAD_OBJ,*PTHREAD_OBJ;


PSOCKET_OBJ GetSocketObj(SOCKET s) {//申请一个套接字对象，初始化他的成员
	PSOCKET_OBJ pSocket=(PSOCKET_OBJ)::GlobalAlloc(GPTR, sizeof(SOCKET_OBJ));
	if (pSocket!=NULL)
	{
		pSocket->s = s;
		pSocket->event = ::WSACreateEvent();
	}
	return pSocket;
}

void FreeSocketObj(PSOCKET_OBJ pSocket) {//释放一个套接字对象

	::CloseHandle(pSocket->event);
	if (pSocket->s!=INVALID_SOCKET)
	{
		::closesocket(pSocket->s);
	}
	::GlobalFree(pSocket);
}





PTHREAD_OBJ g_pThreadList;//指向线程对象，列表表头
CRITICAL_SECTION g_cs;//同步对此全局变量的访问

LONG g_nTatolConnections;//总共连接数量
LONG g_nCurrentConnections;//当前连接数量




//向一个线程的套接字列表中插入一个套接字
BOOL InsertSocketObj(PTHREAD_OBJ pThread, PSOCKET_OBJ pSocket) {
	BOOL bRet = FALSE;
	::EnterCriticalSection(&pThread->cs);
	if (pThread->nSocketCount<WSA_MAXIMUM_WAIT_EVENTS-1)
	{
		if (pThread->pSockHeader==NULL)
		{
			pThread->pSockHeader = pThread->pSockTail = pSocket;
		}
		else
		{
			pThread->pSockTail->pNext = pSocket;
			pThread->pSockTail = pSocket;
		}
		pThread->nSocketCount++;
		bRet = TRUE;
	}
	::LeaveCriticalSection(&pThread->cs);
	//插入成功，说明成功处理了客户的连接请求
	if (bRet)
	{
		::InterlockedIncrement(&g_nTatolConnections);
		::InterlockedIncrement(&g_nCurrentConnections);
	}
	return bRet;
}



PTHREAD_OBJ GetThreadObj() {//申请一个线程对象，初始化他的成员,并将它添加到线程对象列表
	PTHREAD_OBJ pThread = (PTHREAD_OBJ)::GlobalAlloc(GPTR, sizeof(THREAD_OBJ));
	if (pThread!=NULL)
	{
		::InitializeCriticalSection(&pThread->cs);

		//创建一个事件对象，用于指示该线程的句柄数组需要重建
		pThread->events[0]=::WSACreateEvent();


		//将新申请的线程对象添加到列表中
		::EnterCriticalSection(&g_cs);
		pThread->pNext = g_pThreadList;
		g_pThreadList = pThread;
		::LeaveCriticalSection(&g_cs);
	}
	return pThread;
}

void FreeThreadObj(PTHREAD_OBJ pThread)//释放一个线程对象，并将它从线程对象列表移除
{
	//在线程对象列表中查找pThread所指的对象，如果找到就从中移除
	::EnterCriticalSection(&g_cs);
	PTHREAD_OBJ p = g_pThreadList;
	if (p== pThread)
	{
		g_pThreadList = p->pNext;

	}
	else
	{
		while (p!=NULL&& p->pNext!=pThread)
		{
			p = p->pNext;


		}
		if (p!=NULL)
		{
			//此时p是pThread的前一个，既"p->pNext==pThread"
			p->pNext = pThread->pNext;

		}

	}
	::LeaveCriticalSection(&g_cs);

	//释放资源
	::CloseHandle(pThread->events[0]);
	::DeleteCriticalSection(&pThread->cs);
	::GlobalFree(pThread);




}


void RebuildArray(PTHREAD_OBJ pThread) {//重新建立线程对象的wvwnts数组
	::EnterCriticalSection(&pThread->cs);
	PSOCKET_OBJ pSocket = pThread->pSockHeader;
	int n = 1;//从第一个开始写，第零个标识需要重建
	while (pSocket!=NULL)
	{
		pThread->events[n++] = pSocket->event;
		pSocket = pSocket->pNext;

	}
	::LeaveCriticalSection(&pThread->cs);

/*
在程序运行期间，如果有新的套接字添加到这个线程，就使的events[0]事件对象受信，通知线程重新调用RebuildArray函数建立events数组
*/


}

PSOCKET_OBJ FindSocketObj(PTHREAD_OBJ pThread,int nIndex) {//nIndex从1开始
	//在套接字列表中查找
	PSOCKET_OBJ pSocket = pThread->pSockHeader;
	while (--nIndex)
	{
		if (pSocket==NULL)
		{
			return NULL;
		}
		pSocket = pSocket->pNext;
	}
	return pSocket;
}




//从给定线程的套接字对象列表中移除一个套接字对象
void RemoveSocketObj(PTHREAD_OBJ pThread, PSOCKET_OBJ pSocket) {
	::EnterCriticalSection(&pThread->cs);

	//在指定的套接字对象列表中查找指定的套接字对象，找到后将之移除
	PSOCKET_OBJ pTest = pThread->pSockHeader;
	if (pTest==pSocket)
	{
		if (pThread->pSockHeader==pThread->pSockTail)
		{
			pThread->pSockTail = pThread->pSockHeader = pTest->pNext;


		}
		else
		{
			pThread->pSockHeader = pTest->pNext;
		}

	}
	else
	{

		while (pTest!=NULL&&pTest->pNext!=pSocket)
		{
			pTest = pTest->pNext;

		}
		if (pTest!=NULL)
		{
			if (pThread->pSockTail == pSocket) pThread->pSockTail = pTest;
			pTest->pNext = pSocket->pNext;
		}



	}
	pThread->nSocketCount--;
	::LeaveCriticalSection(&pThread->cs);
	::WSASetEvent(pThread->events[0]);//指示线程重建句柄数组
	::InterlockedDecrement(&g_nCurrentConnections);//说明一个连接中断
}



BOOL HandleIO(PTHREAD_OBJ pThread, PSOCKET_OBJ pSocket) {

	//获取具体发生的网络事件
	WSANETWORKEVENTS event;
	::WSAEnumNetworkEvents(pSocket->s, pSocket->event, &event);
	do
	{
		if (event.iErrorCode[FD_READ_BIT]==0)
		{
			char szText[256];
			int nRecv = ::recv(pSocket->s, szText, strlen(szText), 0);
			if (nRecv>0)
			{
				szText[nRecv] = '\0';
				printf("接收到数据：%s\n", szText);

			}
			else
			{
				break;
			}


		}
		else if (event.lNetworkEvents& FD_CLOSE )//套接字关闭
		{
			break;

		}
		else if(event.lNetworkEvents& FD_WRITE)//套接字可写
		{
			if (event.iErrorCode[FD_WRITE_BIT]==0)
			{


			}
			else
			{
				break;
			}


		}
		return TRUE;
		


	} while (FALSE);

	//套接字关闭，或者有错误发生，程序都会转到这里来执行
	RemoveSocketObj(pThread, pSocket);
	FreeSocketObj(pSocket);
	return FALSE;



}



DWORD WINAPI ServerThread(LPVOID lpParam) {
	//取得本线程对象的指针
	PTHREAD_OBJ pThread = (PTHREAD_OBJ)lpParam;
	while (true)
	{
		//等待网络事件
		int nIndex = ::WSAWaitForMultipleEvents(pThread->nSocketCount + 1, pThread->events, FALSE, WSA_INFINITE, FALSE);
		nIndex = nIndex - WSA_WAIT_EVENT_0;

		//查看受信事件对象
		for (int i = nIndex; i <pThread->nSocketCount+1; i++)
		{
			nIndex = ::WSAWaitForMultipleEvents(1, &pThread->events[i], TRUE, 1000, FALSE);
			if (nIndex==WSA_WAIT_FAILED||nIndex==WSA_WAIT_TIMEOUT)
			{
				continue;

			}
			else
			{
				if (i==0)
				{
					//events[0]受信，重建数组
					RebuildArray(pThread);

					//如果没有客户I/O处理，则本线程退出
					if (pThread->nSocketCount==0)
					{
						FreeThreadObj(pThread);
						return 0;

					}
					::WSAResetEvent(pThread->events[0]);


				}
				else//处理网络事件
				{
					//查看对应套接字对象指针，调用HandleIO处理网络事件
					PSOCKET_OBJ pSocket = (PSOCKET_OBJ)FindSocketObj(pThread, i);
					if (pSocket!=NULL)
					{
						if (!HandleIO(pThread,pSocket))
						{
							RebuildArray(pThread);

						}
						else
						{
							printf("Unable to find socket object\n");
						}

					}

				}
			}


		}
		return 0;
	}




}


//将一个套接字对象安排给空闲的线程处理
void AssignToFreeThread(PSOCKET_OBJ pSocket) {
	pSocket->pNext = NULL;
	::EnterCriticalSection(&g_cs);
	PTHREAD_OBJ pThread = g_pThreadList;

	//试图插入到现存线程
	while (pThread!=NULL)
	{
		if (InsertSocketObj(pThread, pSocket)) break;
		pThread = pThread->pNext;
	}
	

	//没有空闲线程，为这个套接字创建新线程
	if (pThread==NULL)
	{
		pThread = GetThreadObj();
		InsertSocketObj(pThread, pSocket);
		::CreateThread(NULL, 0, ServerThread, pThread, 0, NULL);
	}
	::LeaveCriticalSection(&g_cs);
	::WSASetEvent(pThread->events[0]);
}





int main() {
	USHORT nPort = 4567;
	//创建监听套接字
	SOCKET sListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(nPort);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	if (::bind(sListen,(sockaddr*)&sin,sizeof(sin))==SOCKET_ERROR)
	{
		printf("Failed bind()\n");
		return -1;
	}
	::listen(sListen, 200);

	//创建时间对象，并关联到监听套接字
	WSAEVENT event = ::WSACreateEvent();

	::WSAEventSelect(sListen, event, FD_ACCEPT|FD_CLOSE);
	::InitializeCriticalSection(&g_cs);


	//处理客户连接请求，打印状态信息
	while (true)
	{
		int nRet = ::WaitForSingleObject(event, 5 * 1000);
		if (nRet==WAIT_FAILED)
		{
			printf("fAILED wAITfORsINGLEoBJECT()\n");
			break;

		}
		else if(nRet==WSA_WAIT_TIMEOUT)//定时显示状态信息
		{
			printf("\n");
			printf("TatolConnections:%d\n", g_nTatolConnections);
			printf("CurrentConnections:%d\n", g_nCurrentConnections);
			continue;

		}
		else//有新的连接未决
		{

			::ResetEvent(event);
			//循环处理所有未处理连接请求
			while (TRUE)
			{
				sockaddr_in si;
				int nLen = sizeof(si);
				SOCKET sNew = ::accept(sListen, (sockaddr*)&si, &nLen);
				if (sNew==SOCKET_ERROR)
				{
					break;

				}
				PSOCKET_OBJ pSock = GetSocketObj(sNew);
				pSock->addrRemote = si;
				::WSAEventSelect(pSock->s, pSock->event, FD_READ | FD_CLOSE | FD_WRITE);
				AssignToFreeThread(pSock);

			}

		}


	}

	::DeleteCriticalSection(&g_cs);

	return 0;
}