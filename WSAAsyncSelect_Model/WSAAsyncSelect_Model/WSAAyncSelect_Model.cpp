//为了使用WSAAyncSelect I/O模型，程序创建一个隐藏的窗口，窗口函数是WindowsProc
#include"ClientSock.h"
#include"head.h"
#define WM_SOCKET 65535
CInitSock thesock;
LRESULT CALLBACK WindowsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int main() {

	char szClassName[] = "MainWClass";
	WNDCLASSEX wndclass;
	//用描述主窗口的参数填充WNDCLASSEX结构
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
	//创建主窗口
	HWND hWnd = ::CreateWindowEx(0, szClassName, "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);

	if (hWnd==NULL)
	{
		::MessageBox(NULL, "创建窗口出错！", "error", MB_OK);
		return -1;
	}
	USHORT nPort = 4567;//此服务器监听的端口
	//创建监听套接字
	SOCKET sListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(nPort);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	//绑定套接字到本地机器
	if (::bind(sListen,(sockaddr*)&sin,sizeof(sin))==SOCKET_ERROR)
	{
		printf("failed bind()\n");
		return -1;
	}
	//将套接字设为窗口通知消息类型
	::WSAAsyncSelect(sListen, hWnd, SOL_SOCKET, FD_ACCEPT | FD_CLOSE);
	::listen(sListen, 5);

	MSG msg;
	while (::GetMessage(&msg,NULL,0,0))
	{
		//转换为哦键盘消息
		::TranslateMessage(&msg);
		//将消息发送到相应的窗口函数
		::DispatchMessage(&msg);
	}
	return msg.wParam; //当GetMessage返回0时程序结束

}

LRESULT CALLBACK WindowsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

 	switch (uMsg)
	{

	case WM_SOCKET: {
		SOCKET s = wParam;//取得有事件发生的套接字句柄
		//查看释放出错
		if (WSAGETSELECTERROR(lParam)) {
			::closesocket(s);
			return 0;
		}
		//处理发生的事件
		switch (WSAGETSELECTEVENT(lParam))
		{
			case FD_ACCEPT://监听套接字有连接接入
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
					printf("接收数据：%s", szText);
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