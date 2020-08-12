#include"ClientSock.h"
#include"head.h"
CInitSock TheSock;


typedef struct _SOCKET_OBJ {//������¼�ͻ����׽��ֵ���Ϣ
	SOCKET s;//�׽��־��
	HANDLE event;//����׽�����������¼�������
	sockaddr_in addrRemote;//�ͻ��˵�ַ��Ϣ
	_SOCKET_OBJ *pNext;//ָ����һ��SOCKET_OBJ��������һ����
}SOCKET_OBJ, *PSOCKET_OBJ;

typedef struct _THREAD_OBJ {//������¼ÿ���̵߳���Ϣ

	HANDLE events[WSA_MAXIMUM_WAIT_EVENTS];//��¼��ǰҪ�ȴ���¼�¼�����ľ��
	int nSocketCount;//��¼��ǰ�̴߳����׽���������=WSA_MAXIMUM_WAIT_EVENTS
	PSOCKET_OBJ pSockHeader;//��ǰ�̴߳����׽��ֶ����б�pSockHeaderָ���ͷ
	PSOCKET_OBJ pSockTail;//pSockTailָ���β
	CRITICAL_SECTION cs;//�ؼ�����α�����λ����ͬ���Ա��ṹ�ķ���
	_THREAD_OBJ* pNext;//ָ����һ��THREAD_OBJ����Ϊ������Ϊһ����

}THREAD_OBJ,*PTHREAD_OBJ;


PSOCKET_OBJ GetSocketObj(SOCKET s) {//����һ���׽��ֶ��󣬳�ʼ�����ĳ�Ա
	PSOCKET_OBJ pSocket=(PSOCKET_OBJ)::GlobalAlloc(GPTR, sizeof(SOCKET_OBJ));
	if (pSocket!=NULL)
	{
		pSocket->s = s;
		pSocket->event = ::WSACreateEvent();
	}
	return pSocket;
}

void FreeSocketObj(PSOCKET_OBJ pSocket) {//�ͷ�һ���׽��ֶ���

	::CloseHandle(pSocket->event);
	if (pSocket->s!=INVALID_SOCKET)
	{
		::closesocket(pSocket->s);
	}
	::GlobalFree(pSocket);
}





PTHREAD_OBJ g_pThreadList;//ָ���̶߳����б��ͷ
CRITICAL_SECTION g_cs;//ͬ���Դ�ȫ�ֱ����ķ���

LONG g_nTatolConnections;//�ܹ���������
LONG g_nCurrentConnections;//��ǰ��������




//��һ���̵߳��׽����б��в���һ���׽���
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
	//����ɹ���˵���ɹ������˿ͻ�����������
	if (bRet)
	{
		::InterlockedIncrement(&g_nTatolConnections);
		::InterlockedIncrement(&g_nCurrentConnections);
	}
	return bRet;
}



PTHREAD_OBJ GetThreadObj() {//����һ���̶߳��󣬳�ʼ�����ĳ�Ա,��������ӵ��̶߳����б�
	PTHREAD_OBJ pThread = (PTHREAD_OBJ)::GlobalAlloc(GPTR, sizeof(THREAD_OBJ));
	if (pThread!=NULL)
	{
		::InitializeCriticalSection(&pThread->cs);

		//����һ���¼���������ָʾ���̵߳ľ��������Ҫ�ؽ�
		pThread->events[0]=::WSACreateEvent();


		//����������̶߳�����ӵ��б���
		::EnterCriticalSection(&g_cs);
		pThread->pNext = g_pThreadList;
		g_pThreadList = pThread;
		::LeaveCriticalSection(&g_cs);
	}
	return pThread;
}

void FreeThreadObj(PTHREAD_OBJ pThread)//�ͷ�һ���̶߳��󣬲��������̶߳����б��Ƴ�
{
	//���̶߳����б��в���pThread��ָ�Ķ�������ҵ��ʹ����Ƴ�
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
			//��ʱp��pThread��ǰһ������"p->pNext==pThread"
			p->pNext = pThread->pNext;

		}

	}
	::LeaveCriticalSection(&g_cs);

	//�ͷ���Դ
	::CloseHandle(pThread->events[0]);
	::DeleteCriticalSection(&pThread->cs);
	::GlobalFree(pThread);




}


void RebuildArray(PTHREAD_OBJ pThread) {//���½����̶߳����wvwnts����
	::EnterCriticalSection(&pThread->cs);
	PSOCKET_OBJ pSocket = pThread->pSockHeader;
	int n = 1;//�ӵ�һ����ʼд���������ʶ��Ҫ�ؽ�
	while (pSocket!=NULL)
	{
		pThread->events[n++] = pSocket->event;
		pSocket = pSocket->pNext;

	}
	::LeaveCriticalSection(&pThread->cs);

/*
�ڳ��������ڼ䣬������µ��׽�����ӵ�����̣߳���ʹ��events[0]�¼��������ţ�֪ͨ�߳����µ���RebuildArray��������events����
*/


}

PSOCKET_OBJ FindSocketObj(PTHREAD_OBJ pThread,int nIndex) {//nIndex��1��ʼ
	//���׽����б��в���
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




//�Ӹ����̵߳��׽��ֶ����б����Ƴ�һ���׽��ֶ���
void RemoveSocketObj(PTHREAD_OBJ pThread, PSOCKET_OBJ pSocket) {
	::EnterCriticalSection(&pThread->cs);

	//��ָ�����׽��ֶ����б��в���ָ�����׽��ֶ����ҵ���֮�Ƴ�
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
	::WSASetEvent(pThread->events[0]);//ָʾ�߳��ؽ��������
	::InterlockedDecrement(&g_nCurrentConnections);//˵��һ�������ж�
}



BOOL HandleIO(PTHREAD_OBJ pThread, PSOCKET_OBJ pSocket) {

	//��ȡ���巢���������¼�
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
				printf("���յ����ݣ�%s\n", szText);

			}
			else
			{
				break;
			}


		}
		else if (event.lNetworkEvents& FD_CLOSE )//�׽��ֹر�
		{
			break;

		}
		else if(event.lNetworkEvents& FD_WRITE)//�׽��ֿ�д
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

	//�׽��ֹرգ������д����������򶼻�ת��������ִ��
	RemoveSocketObj(pThread, pSocket);
	FreeSocketObj(pSocket);
	return FALSE;



}



DWORD WINAPI ServerThread(LPVOID lpParam) {
	//ȡ�ñ��̶߳����ָ��
	PTHREAD_OBJ pThread = (PTHREAD_OBJ)lpParam;
	while (true)
	{
		//�ȴ������¼�
		int nIndex = ::WSAWaitForMultipleEvents(pThread->nSocketCount + 1, pThread->events, FALSE, WSA_INFINITE, FALSE);
		nIndex = nIndex - WSA_WAIT_EVENT_0;

		//�鿴�����¼�����
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
					//events[0]���ţ��ؽ�����
					RebuildArray(pThread);

					//���û�пͻ�I/O�������߳��˳�
					if (pThread->nSocketCount==0)
					{
						FreeThreadObj(pThread);
						return 0;

					}
					::WSAResetEvent(pThread->events[0]);


				}
				else//���������¼�
				{
					//�鿴��Ӧ�׽��ֶ���ָ�룬����HandleIO���������¼�
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


//��һ���׽��ֶ����Ÿ����е��̴߳���
void AssignToFreeThread(PSOCKET_OBJ pSocket) {
	pSocket->pNext = NULL;
	::EnterCriticalSection(&g_cs);
	PTHREAD_OBJ pThread = g_pThreadList;

	//��ͼ���뵽�ִ��߳�
	while (pThread!=NULL)
	{
		if (InsertSocketObj(pThread, pSocket)) break;
		pThread = pThread->pNext;
	}
	

	//û�п����̣߳�Ϊ����׽��ִ������߳�
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
	//���������׽���
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

	//����ʱ����󣬲������������׽���
	WSAEVENT event = ::WSACreateEvent();

	::WSAEventSelect(sListen, event, FD_ACCEPT|FD_CLOSE);
	::InitializeCriticalSection(&g_cs);


	//����ͻ��������󣬴�ӡ״̬��Ϣ
	while (true)
	{
		int nRet = ::WaitForSingleObject(event, 5 * 1000);
		if (nRet==WAIT_FAILED)
		{
			printf("fAILED wAITfORsINGLEoBJECT()\n");
			break;

		}
		else if(nRet==WSA_WAIT_TIMEOUT)//��ʱ��ʾ״̬��Ϣ
		{
			printf("\n");
			printf("TatolConnections:%d\n", g_nTatolConnections);
			printf("CurrentConnections:%d\n", g_nCurrentConnections);
			continue;

		}
		else//���µ�����δ��
		{

			::ResetEvent(event);
			//ѭ����������δ������������
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