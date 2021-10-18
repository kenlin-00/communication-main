
#ifndef __NGX_COMM_H__
#define __NGX_COMM_H__


#define _PKG_MAX_LENGTH     30000 

//通信 收包状态定义
#define _PKG_HD_INIT         0  
#define _PKG_HD_RECVING      1  
#define _PKG_BD_INIT         2 
#define _PKG_BD_RECVING      3  
//#define _PKG_RV_FINISHED     4  

#define _DATA_BUFSIZE_       20  
//结构定义------------------------------------
#pragma pack (1)

//一些和网络通讯相关的结构放在这里
//包头结构
typedef struct _COMM_PKG_HEADER
{
	unsigned short pkgLen;    

	unsigned short msgCode;   
}COMM_PKG_HEADER,*LPCOMM_PKG_HEADER;


#pragma pack() //取消指定对齐，恢复缺省对齐


#endif
