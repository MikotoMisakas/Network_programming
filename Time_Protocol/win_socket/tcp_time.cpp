#include"head.h"
#include"ClientSock.h"
CInitSock initSocket;

void SetTimeFromTP(ULONG ulTime)//����ʱ��Э�鷵�ص�ʱ������ϵͳʱ��
{
	//windows�ļ�ʱ����һ��64λ��ֵ������1601��1��1������ʮ���㵽���ڵ�ʱ����
	//��λ��1/1000 0000�룬1000���֮һ��
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
	//Ȼ��ʹ��Time Protocolʹ�û�׼ʱ�������ȥʱ�䣬��ulTime
	LONGLONG *pLLong = (LONGLONG*)&ft;

	//ע���ļ���ʱ�䵥λֵǧ���֮һ��
	*pLLong += (LONGLONG)10000000 * ulTime;

	//�ڽ�ʱ��ת������������ϵͳʱ��
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


	//��дԶ�̵�ַ��Ϣ�����ӵ�ʱ�������
	sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(123);

	servAddr.sin_addr.S_un.S_addr = inet_addr("202.120.2.101");
	if (::connect(s, (sockaddr*)&servAddr, sizeof(servAddr)) == -1)
	{
		printf("Failed connect()\n");
		return 0;

	}

	//�ȴ�����ʱ��Э�鷵�ص�ʱ�䣬ѧϰ��windows I/Oģ��֮�����ʹ���첽I/O���Ա����ó�ʱ
	ULONG ulTime = 0;
	int nRecv = ::recv(s, (char*)&ulTime, sizeof(ulTime), 0);
	if (nRecv > 0)
	{
		ulTime = ntohl(ulTime);
		SetTimeFromTP(ulTime);
		printf("�ɹ���ʱ���������ʱ��ͬ��\n");

	}
	else
	{
		printf("ʱ�����������ȷ����ǰʱ�䣡\n");
	}
	::closesocket(s);
	return 0;
}