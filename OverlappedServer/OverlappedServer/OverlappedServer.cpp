#include"head.h"
#include"ClientSock.h"
CInitSock theSock;
HANDLE g_events[WSA_MAXIMUM_WAIT_EVENTS];//I/O事件句柄数组
int g_nBufferCount;//上数组句柄有效的数量

int BUFFER_SIZE = 1256;
/*
为每个套接字创建一个SOCKET_OBJ对象，以便记录与之相关的信息。SOCKET_OBJ结构的定义如下
*/
typedef struct _SOCKET_OBJ {
	SOCKET s;//套接字句柄
	int nOutstandingOps;//记录此套接字上重叠I/O的数量
	LPFN_ACCEPTEX lpfnAcceptEx;//扩展函数AcceptEx的指针（仅对监听套接字而言）
}SOCKET_OBJ,*PSOCKET_OBJ;

//所有的重叠I/O都需要提交到特定的套接字上，如果在这些I/O完成之前，对方关闭了连接或者连接发生错误，就需要释放对应的SOCKET_OBJ对象
//但是释放之前必须保证套接字再也没有重叠I/O了，既nOutstanding的值为0


//申请和释放套接字的函数
PSOCKET_OBJ GetSocketObj(SOCKET s) {

	PSOCKET_OBJ pSocket = (PSOCKET_OBJ)::GlobalAlloc(GPTR, sizeof(SOCKET_OBJ));
	if (pSocket!=NULL)
	{
		pSocket->s = s;
	}
	return pSocket;
}

void FreeSocketObj(PSOCKET_OBJ pSocket) {
	if (pSocket->s != INVALID_SOCKET)
		::closesocket(pSocket->s);
	::GlobalFree(pSocket);
}


//缓冲区对象BUFFER_OBJ非常重要，记录了重叠I/O的所有属性
typedef struct _BUFFER_OBJ {
	OVERLAPPED ol;//重叠结构
	char* buff;//send recv AcceptEx所使用的缓冲区
	int nLen;//buff的长度
	PSOCKET_OBJ pSocket;//此I/O所属套接字对象
	int nOperation;//提交的操作类型
	#define OP_ACCEPT 1
	#define OP_READ	  2
	#define OP_WRITE  3
	SOCKET sAccept;//用来保存AcceptEx接受客户端套接字，仅对监听套接字而言
	_BUFFER_OBJ *pNext;
}BUFFER_OBJ,*PBUFFER_OBJ;
PBUFFER_OBJ g_pBufferHead;//记录缓冲区对象组成表的地址
PBUFFER_OBJ g_pBufferTail;

//每次调用重叠I/O函数如（WSASend），都要申请一个BUFFER_OBJ对象，以便记录I/O信息，如缓冲区地址操作类型等，在I/O完成后在释放这个对象
PBUFFER_OBJ GetBufferObj(PSOCKET_OBJ pSocket, ULONG nLen) {
	if (g_nBufferCount>WSA_MAXIMUM_WAIT_EVENTS-1)
	{
		return NULL;
	}
	PBUFFER_OBJ pBuffer = (PBUFFER_OBJ)::GlobalAlloc(GPTR, sizeof(BUFFER_OBJ));
	if (pBuffer!=NULL)
	{
		pBuffer->buff = (char*)::GlobalAlloc(GPTR, nLen);
		pBuffer->ol.hEvent = ::WSACreateEvent();
		pBuffer->pSocket = pSocket;
		pBuffer->sAccept = INVALID_SOCKET;
		//将新的BUFFER_OBJ添加进列表中
		if (g_pBufferHead == NULL)
		{
			g_pBufferHead = g_pBufferTail = pBuffer;
		}
		else
		{
			g_pBufferTail->pNext = pBuffer;
			g_pBufferTail = pBuffer;
		}
		g_events[++g_nBufferCount] = pBuffer->ol.hEvent;
	}
	return pBuffer;
}

//从列表中移除BUFFER_OBJ对象
void FreeBufferObj(PBUFFER_OBJ pBuffer) {
	PBUFFER_OBJ pTest = g_pBufferHead;
	BOOL bFind = FALSE;
	if (pTest==pBuffer)
	{
		g_pBufferHead = g_pBufferTail = NULL;
		bFind = TRUE;
	}
	else
	{
		while (pTest != NULL && pTest->pNext != pBuffer)
			pTest = pTest->pNext;
		if (pTest!=NULL)
		{
			pTest->pNext = pBuffer->pNext;
			if (pTest->pNext == NULL)
				g_pBufferTail = pTest;
			bFind = TRUE;
		}
	}
	//释放它所占用的空间
	if (bFind)
	{
		g_nBufferCount--;
		::CloseHandle(pBuffer->ol.hEvent);
		::GlobalFree(pBuffer->buff);
		::GlobalFree(pBuffer);
	}
}


//提交重叠I/O时，传递的参数有重叠结构ol和缓冲区指针buff，在重叠I/O完成后，得到的是受信事件对象的句柄
//还需要根据此句柄找到对应的BUFFER_OBJ对象，因此定义如下FindBufferObj函数，以在缓冲区列表中查找BUFFER_OBJ对象
PBUFFER_OBJ FindBufferObj(HANDLE hEvent) {
	PBUFFER_OBJ pBuffer = g_pBufferHead;
	while (pBuffer!=NULL)
	{
		if (pBuffer->ol.hEvent == hEvent)
			break;
		pBuffer = pBuffer->pNext;
	}
	return pBuffer;
}







//更新事件句柄数组g_events中的内容
void RebuildArray() {
	PBUFFER_OBJ pBuffer = g_pBufferHead;
	int i = 1;
	while (pBuffer!=NULL)
	{
		g_events[i++] = pBuffer->ol.hEvent;
		pBuffer = pBuffer->pNext;
	}
}

BOOL PostAccept(PBUFFER_OBJ pBuffer) {
	PSOCKET_OBJ pSocket = pBuffer->pSocket;
	if (pSocket->lpfnAcceptEx != NULL) {
		//设置I/O类型，增加套接字上重叠IO计数
		pBuffer->nOperation = OP_ACCEPT;
		pSocket->nOutstandingOps++;

		//投递此重叠I/O
		DWORD dwBytes;
		pBuffer->sAccept = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0,WSA_FLAG_OVERLAPPED);
		BOOL b = pSocket->lpfnAcceptEx(pSocket->s,
			pBuffer->sAccept,
			pBuffer->buff,
			BUFFER_SIZE - ((sizeof(sockaddr_in) + 16) * 2),
			sizeof(sockaddr_in) + 16,
			sizeof(sockaddr_in) + 16,
			&dwBytes,
			&pBuffer->ol
		);
		if (!b)
		{
			if (::WSAGetLastError() != WSA_IO_PENDING)
				return FALSE;
		}
		return TRUE;
	}
	return FALSE;


}


BOOL PostRecv(PBUFFER_OBJ pBuffer) {
	//设置I/O类型，增加套接字上的重叠I/O计数
	pBuffer->nOperation = OP_READ;
	pBuffer->pSocket->nOutstandingOps++;

	//投递此重叠I/O
	DWORD dwBytes;
	DWORD dwFlags = 0;
	WSABUF buf;
	buf.buf = pBuffer->buff;
	buf.len = pBuffer->nLen;
	if (::WSARecv(pBuffer->pSocket->s,&buf,1,&dwBytes,&dwFlags,&pBuffer->ol,NULL)!=NO_ERROR)
	{
		if (::WSAGetLastError() != WSA_IO_PENDING)
			return FALSE;
	}
}

BOOL PostSend(PBUFFER_OBJ pBuffer) {
	//设置I/O类型，增加套接字上的重叠I/O计数
	pBuffer->nOperation = OP_WRITE;
	pBuffer->pSocket->nOutstandingOps++;

	//投递此重叠I/O
	DWORD dwBytes;
	DWORD dwFlags = 0;
	WSABUF buf;
	buf.buf = pBuffer->buff;
	buf.len = pBuffer->nLen;
	if (::WSASend(pBuffer->pSocket->s,
		&buf, 1, &dwBytes, dwFlags, &pBuffer->ol, NULL
	) != NO_ERROR) {
		if (::WSAGetLastError() != WSA_IO_PENDING)
			return FALSE;
	}
	return TRUE;
}

BOOL HandleIO(PBUFFER_OBJ pBuffer) {

	PSOCKET_OBJ pSocket = pBuffer->pSocket;//从BUFFER_OBJ对象中提取SOCKET_OBJ对象指针，可以方便引用
	pSocket->nOutstandingOps--;

	//获取重叠操作结果
	DWORD dwTrans;
	DWORD dwFlags;
	BOOL bRet = ::WSAGetOverlappedResult(pSocket->s, &pBuffer->ol, &dwTrans, FALSE, &dwFlags);
	if (!bRet)
	{
		//在此套接字有错误发生，因此关闭套接字，移除此缓冲区对象
		//如果没有其他抛出的IO请求，释放此缓冲区对象，否则等待此套接字上的其他I/O也完成
		if (pSocket->s!=INVALID_SOCKET)
		{
			::closesocket(pSocket->s);
			pSocket->s = INVALID_SOCKET;

		}
		if (pSocket->nOutstandingOps == 0)
			FreeBufferObj(pBuffer);
		FreeBufferObj(pBuffer);
		return FALSE;

		



	}
	switch (pBuffer->nOperation)
	{
	case OP_ACCEPT://接收到一个新连接，并接收到对方发来的第一个封包
	{
		//为新客户创建一个新的SOCKET_OBJ对象
		PSOCKET_OBJ pClient = GetSocketObj(pBuffer->sAccept);
		//为发送数据创建一个BUFFER_OBJ对象，这个对象会在套接字出错或者关闭时，释放。
		PBUFFER_OBJ pSend = GetBufferObj(pClient, BUFFER_SIZE);
		if (pSend==NULL)
		{
			printf("Too MUCH CONNECTIONS!\n");
			FreeSocketObj(pClient);
			return FALSE;
		}
		RebuildArray();

		//将数据复制到发送缓冲区
		pSend->nLen = dwTrans;
		memcpy(pSend->buff, pBuffer->buff, dwTrans);


		//投递此发送I/O，将数据回显给客户
		if (!PostSend(pSend))
		{
			//出错释放上面两个刚申请的对象
			FreeSocketObj(pSocket);
			FreeBufferObj(pSend);
			return FALSE;
		}
		//继续投递接受I/O
		PostAccept(pBuffer);




	}
	break;

	case OP_READ: {

		if (dwTrans>0)
		{
			//创建一个原来的缓冲区以发送数据，这里就使用原来的缓冲区
			PBUFFER_OBJ pSend = pBuffer;
			pSend->nLen = dwTrans;
			//将数据回显给客户
			PostSend(pSend);

		}
		else
		{
			//套接字关闭
			//先关闭套接字以便在此套接字上投递的其他I/O也返回
			if (pSocket->s!=INVALID_SOCKET)
			{
				::closesocket(pSocket->s);
				pSocket->s = INVALID_SOCKET;

			}
			if (pSocket->nOutstandingOps == 0)
				FreeSocketObj(pSocket);
			FreeBufferObj(pBuffer);
			return FALSE;

			
		}


	}
	break;

	case OP_WRITE: {
		//发送数据完成
		if (dwTrans>0)
		{
			//继续使用这个缓冲区投递接受数据的请求
			pBuffer->nLen = BUFFER_SIZE;
			PostRecv(pBuffer);

		}
		else
		{
			//同样需要先关闭套接字
			if (pSocket->s!=INVALID_SOCKET)
			{
				::closesocket(pSocket->s);
				pSocket->s = INVALID_SOCKET;
			}
			if (pSocket->nOutstandingOps == 0)
				FreeSocketObj(pSocket);
			FreeBufferObj(pBuffer);
			return FALSE;

			


		}



	}
				   break;



	}

	return TRUE;


}



int main() {

//创建监听套接字，绑定到本地端口，进入监听模式
	int nPort = 4567;
	SOCKET sListen = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN si;
	si.sin_family = AF_INET;
	si.sin_port = ::ntohs(4567);
	si.sin_addr.S_un.S_addr = INADDR_ANY;
	::bind(sListen, (sockaddr*)&si, sizeof(si));
	int aa = GetLastError();
	::listen(sListen, 200);


	//为监听套接字创建一个SOCKET_OBJ对象
	PSOCKET_OBJ pListen = GetSocketObj(sListen);

	//加载扩展函数AcceptEx
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes;
	int ss=WSAIoctl(pListen->s,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx,
		sizeof(GuidAcceptEx),
		&pListen->lpfnAcceptEx,
		sizeof(pListen->lpfnAcceptEx),
		&dwBytes,
		NULL, NULL
	);
	if (ss == SOCKET_ERROR) {
		wprintf(L"WSAIoctl failed with error: %u\n", WSAGetLastError());

		WSACleanup();
		return 1;
	}
	//创建用来重新建立g_events数组的事件对象
	g_events[0] = ::WSACreateEvent();

	//再次可以投递多个接受I/O请求
	
	for (int i =0 ; i < 5; i++) {
		PostAccept(GetBufferObj(pListen, BUFFER_SIZE));
	}

	while (true)
	{
		int nIndex = ::WSAWaitForMultipleEvents(g_nBufferCount + 1, g_events, FALSE, WSA_INFINITE, FALSE);



		if (nIndex==WSA_WAIT_FAILED)
		{
			printf("WSAWaitForMultipleEvents() failed\n");
			break;
		}
		nIndex = nIndex - WSA_WAIT_EVENT_0;
		for (int i = 0; i < nIndex+1; i++)
		{
			int nRet = ::WSAWaitForMultipleEvents(1, &g_events[i], FALSE, 0, FALSE);
			int aa=WSAGetLastError();
			//int nRet = ::WSAWaitForMultipleEvents(1, &g_events[i], TRUE, 2000, TRUE);
			if (nRet==WSA_WAIT_TIMEOUT)
			{
				continue;
			}
			else
			{
				::WSAResetEvent(g_events[i]);
				//重新建立g_events数组
				if (i==0)
				{
					RebuildArray();
					continue;

				}
				//处理这个I/O
				PBUFFER_OBJ pBuffer = FindBufferObj(g_events[i]);
				if (pBuffer!=NULL)
				{
					if (!HandleIO(pBuffer))
					{
						RebuildArray();
					}
				}
			}
		}
	}
	return 0;
}