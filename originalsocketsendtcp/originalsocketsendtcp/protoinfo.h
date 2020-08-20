//////////////////////////////////////////////////
// protoinfo.h文件

/*

定义协议格式
定义协议中使用的宏

 */
#include"head.h"




#define ETHERTYPE_IP    0x0800
#define ETHERTYPE_ARP   0x0806

///提供IP头和TCP头结构
#define URG 0X20
#define ACK 0X10
#define PSH 0X08
#define RST 0X04
#define SYN 0X02
#define FIN 0X01

typedef struct _iphdr {
	unsigned char h_lenver;//4位首部长度+4位IP版本号
	unsigned char tos;//8位服务类型
	unsigned short total_len;//16位总长度
	unsigned short ident;//16位标志
	unsigned frag_and_flags;//3位标志
	unsigned char ttl;//8位生存时间
	unsigned char proto;//8位协议tcp、udp等
	unsigned short checksum;//16位ip首部校验和
	unsigned int sourceIP;//32位源IP
	unsigned int destIP;//32位目的IP
}IP_HEADER;

typedef struct psd_hdr {
	unsigned long saddr;//源地址
	unsigned long daddr;//目的地址
	char mbz;
	char ptcl;//协议类型
	unsigned short tcpl;//TCp长度
}PSD_HEADER;

typedef struct _tcphdr {//定义TCP首部
	USHORT th_sport;//16位源端口号
	USHORT th_dport;//位目的端口号
	unsigned int th_seq;//32位序列号
	unsigned int th_ack;//32位确认号
	unsigned char th_lenres;//4位部首长度和6位保留字
	unsigned char th_flag;//6位标志位
	USHORT th_win;//16位窗口大小
	USHORT th_sum;//16位校验和
	USHORT th_urp;//16位紧急数据偏移量
}TCP_HEADER;
//TCP伪首并不存在只是为了计算校验和