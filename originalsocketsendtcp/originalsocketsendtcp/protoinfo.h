//////////////////////////////////////////////////
// protoinfo.h�ļ�

/*

����Э���ʽ
����Э����ʹ�õĺ�

 */
#include"head.h"




#define ETHERTYPE_IP    0x0800
#define ETHERTYPE_ARP   0x0806

///�ṩIPͷ��TCPͷ�ṹ
#define URG 0X20
#define ACK 0X10
#define PSH 0X08
#define RST 0X04
#define SYN 0X02
#define FIN 0X01

typedef struct _iphdr {
	unsigned char h_lenver;//4λ�ײ�����+4λIP�汾��
	unsigned char tos;//8λ��������
	unsigned short total_len;//16λ�ܳ���
	unsigned short ident;//16λ��־
	unsigned frag_and_flags;//3λ��־
	unsigned char ttl;//8λ����ʱ��
	unsigned char proto;//8λЭ��tcp��udp��
	unsigned short checksum;//16λip�ײ�У���
	unsigned int sourceIP;//32λԴIP
	unsigned int destIP;//32λĿ��IP
}IP_HEADER;

typedef struct psd_hdr {
	unsigned long saddr;//Դ��ַ
	unsigned long daddr;//Ŀ�ĵ�ַ
	char mbz;
	char ptcl;//Э������
	unsigned short tcpl;//TCp����
}PSD_HEADER;

typedef struct _tcphdr {//����TCP�ײ�
	USHORT th_sport;//16λԴ�˿ں�
	USHORT th_dport;//λĿ�Ķ˿ں�
	unsigned int th_seq;//32λ���к�
	unsigned int th_ack;//32λȷ�Ϻ�
	unsigned char th_lenres;//4λ���׳��Ⱥ�6λ������
	unsigned char th_flag;//6λ��־λ
	USHORT th_win;//16λ���ڴ�С
	USHORT th_sum;//16λУ���
	USHORT th_urp;//16λ��������ƫ����
}TCP_HEADER;
//TCPα�ײ�������ֻ��Ϊ�˼���У���