//Ϊ��ʹ��WSAAyncSelect I/Oģ�ͣ����򴴽�һ�����صĴ��ڣ����ں�����WindowsProc
#include"ClientSock.h"
#include"head.h"
#define WM_SOCKET 65535
CInitSock thesock;
LRESULT CALLBACK WindowsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int main() {

	char szClassName[] = "MainWClass";
	WNDCLASSEX wndclass;
	//�����������ڵĲ������WNDCLASSEX�ṹ
	wndclass.cbSize = sizeof(wndclass);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WindowsProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = NULL;
	wndclass.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)::GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szClassName;
	wndclass.hIconSm = NULL;
	::RegisterClassEx(&wndclass);
	//����������
	HWND hWnd = ::CreateWindowEx(0, szClassName, "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);

	if (hWnd==NULL)
	{
		::MessageBox(NULL, "�������ڳ���", "error", MB_OK);
		return -1;
	}
	USHORT nPort = 4567;//�˷����������Ķ˿�
	//���������׽���
	SOCKET sListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(nPort);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	//���׽��ֵ����ػ���
	if (::bind(sListen,(sockaddr*)&sin,sizeof(sin))==SOCKET_ERROR)
	{
		printf("failed bind()\n");
		return -1;
	}
	//���׽�����Ϊ����֪ͨ��Ϣ����
	::WSAAsyncSelect(sListen, hWnd, SOL_SOCKET, FD_ACCEPT | FD_CLOSE);
	::listen(sListen, 5);

	MSG msg;
	while (::GetMessage(&msg,NULL,0,0))
	{
		//ת��ΪŶ������Ϣ
		::TranslateMessage(&msg);
		//����Ϣ���͵���Ӧ�Ĵ��ں���
		::DispatchMessage(&msg);
	}
	return msg.wParam; //��GetMessage����0ʱ�������

}

LRESULT CALLBACK WindowsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

 	switch (uMsg)
	{

	case WM_SOCKET: {
		SOCKET s = wParam;//ȡ�����¼��������׽��־��
		//�鿴�ͷų���
		if (WSAGETSELECTERROR(lParam)) {
			::closesocket(s);
			return 0;
		}
		//���������¼�
		switch (WSAGETSELECTEVENT(lParam))
		{
			case FD_ACCEPT://�����׽��������ӽ���
			{
				SOCKET client = ::accept(s, NULL, NULL);
				::WSAAsyncSelect(client, hWnd, SOL_SOCKET, FD_READ | FD_WRITE | FD_CLOSE);

			}
				break;
			case FD_WRITE:{}
					  break;
			case FD_READ: {
				char szText[1024] = { 0 };
				if (::recv(s, szText, 1024, 0) == -1)
				{
					::closesocket(s);
				}
				else
				{
					printf("�������ݣ�%s", szText);
				}
			}
				break;
			case FD_CLOSE: {
				::closesocket(s);
			}
						   break;			
			}
		}
					return 0;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}