
#ifndef __NGX_SOCKET_H__
#define __NGX_SOCKET_H__

#include <vector>       //vector
#include <list>         //list
#include <sys/epoll.h>  //epoll
#include <sys/socket.h>
#include <pthread.h>    //多线程
#include <semaphore.h>  //信号量 
#include <atomic>       //c++11里的原子操作

#include "ngx_comm.h"

//一些宏定义放在这里-----------------------------------------------------------
#define NGX_LISTEN_BACKLOG  511    //已完成连接队列，nginx给511，我们也先按照这个来：不懂这个数字的同学参考第五章第四节
#define NGX_MAX_EVENTS      512    //epoll_wait一次最多接收这么多个事件，nginx中缺省是512，我们这里固定给成512就行，没太大必要修改

typedef struct ngx_listening_s   ngx_listening_t, *lpngx_listening_t;
typedef struct ngx_connection_s  ngx_connection_t,*lpngx_connection_t;
typedef class  CSocekt           CSocekt;

typedef void (CSocekt::*ngx_event_handler_pt)(lpngx_connection_t c); //定义成员函数指针

//--------------------------------------------
//一些专用结构定义放在这里，暂时不考虑放ngx_global.h里了
struct ngx_listening_s  //和监听端口有关的结构
{
	int                       port;        //监听的端口号
	int                       fd;          //套接字句柄socket
	lpngx_connection_t        connection;  //连接池中的一个连接，注意这是个指针 
};

//以下三个结构是非常重要的三个结构，我们遵从官方nginx的写法；
//(1)该结构表示一个TCP连接【客户端主动发起的、Nginx服务器被动接受的TCP连接】
struct ngx_connection_s
{		
	ngx_connection_s();                                      //构造函数
	virtual ~ngx_connection_s();                             //析构函数
	void GetOneToUse();                                      //分配出去的时候初始化一些内容
	void PutOneToFree();                                     //回收回来的时候做一些事情


	int                       fd;                            //套接字句柄socket
	lpngx_listening_t         listening;                     //如果这个链接被分配给了一个监听套接字，那么这个里边就指向监听套接字对应的那个lpngx_listening_t的内存首地址		

	//------------------------------------	
	//unsigned                  instance:1;                    //【位域】失效标志位：0：有效，1：失效【这个是官方nginx提供，到底有什么用，ngx_epoll_process_events()中详解】  
	uint64_t                  iCurrsequence;                 //我引入的一个序号，每次分配出去时+1，此法也有可能在一定程度上检测错包废包，具体怎么用，用到了再说
	struct sockaddr           s_sockaddr;                    //保存对方地址信息用的
	//char                      addr_text[100]; //地址的文本信息，100足够，一般其实如果是ipv4地址，255.255.255.255，其实只需要20字节就够

	//和读有关的标志-----------------------
	//uint8_t                   r_ready;        //读准备好标记【暂时没闹明白官方要怎么用，所以先注释掉】
	//uint8_t                   w_ready;        //写准备好标记

	ngx_event_handler_pt      rhandler;                       //读事件的相关处理方法
	ngx_event_handler_pt      whandler;                       //写事件的相关处理方法

	//和epoll事件有关
	uint32_t                  events;                         //和epoll事件有关  
	
	//和收包有关
	unsigned char             curStat;                        //当前收包的状态
	char                      dataHeadInfo[_DATA_BUFSIZE_];   //用于保存收到的数据的包头信息			
	char                      *precvbuf;                      //接收数据的缓冲区的头指针，对收到不全的包非常有用，看具体应用的代码
	unsigned int              irecvlen;                       //要收到多少数据，由这个变量指定，和precvbuf配套使用，看具体应用的代码
	char                      *precvMemPointer;               //new出来的用于收包的内存首地址，释放用的

	pthread_mutex_t           logicPorcMutex;                 //逻辑处理相关的互斥量      

	//和发包有关
	std::atomic<int>          iThrowsendCount;                //发送消息，如果发送缓冲区满了，则需要通过epoll事件来驱动消息的继续发送，所以如果发送缓冲区满，则用这个变量标记
	char                      *psendMemPointer;               //发送完成后释放用的，整个数据的头指针，其实是 消息头 + 包头 + 包体
	char                      *psendbuf;                      //发送数据的缓冲区的头指针，开始 其实是包头+包体
	unsigned int              isendlen;                       //要发送多少数据

	//和回收有关
	time_t                    inRecyTime;                     //入到资源回收站里去的时间

	//--------------------------------------------------
	lpngx_connection_t        next;                           //这是个指针，指向下一个本类型对象，用于把空闲的连接池对象串起来构成一个单向链表，方便取用
};

//消息头，引入的目的是当收到数据包时，额外记录一些内容以备将来使用
typedef struct _STRUC_MSG_HEADER
{
	lpngx_connection_t pConn;         //记录对应的链接，注意这是个指针
	uint64_t           iCurrsequence; //收到数据包时记录对应连接的序号，将来能用于比较是否连接已经作废用
	//......其他以后扩展	
}STRUC_MSG_HEADER,*LPSTRUC_MSG_HEADER;

//------------------------------------
//socket相关类
class CSocekt
{
public:
	CSocekt();                                                            
	virtual ~CSocekt();                                                  
	virtual bool Initialize();                                            
	virtual bool Initialize_subproc();                                   
	virtual void Shutdown_subproc();                                      

public:
	virtual void threadRecvProcFunc(char *pMsgBuf);                       //处理客户端请求，虚函数，因为将来可以考虑自己来写子类继承本类
public:	
	int  ngx_epoll_init();                                                //epoll功能初始化	
	//int  ngx_epoll_add_event(int fd,int readevent,int writeevent,uint32_t otherflag,uint32_t eventtype,lpngx_connection_t pConn);     
	                                                                      //epoll增加事件
	int  ngx_epoll_process_events(int timer);                             //epoll等待接收和处理事件

	int ngx_epoll_oper_event(int fd,uint32_t eventtype,uint32_t flag,int bcaction,lpngx_connection_t pConn); 
	                                                                      //epoll操作事件
	
protected:
	//数据发送相关
	void msgSend(char *psendbuf);                                         //把数据扔到待发送对列中 

private:	
	void ReadConf();                                                   	
	bool ngx_open_listening_sockets();                                    
	void ngx_close_listening_sockets();                                   
	bool setnonblocking(int sockfd);                                    

	//一些业务处理函数handler
	void ngx_event_accept(lpngx_connection_t oldc);                       
	void ngx_read_request_handler(lpngx_connection_t pConn);              
	void ngx_write_request_handler(lpngx_connection_t pConn);             
	void ngx_close_connection(lpngx_connection_t pConn);                

	ssize_t recvproc(lpngx_connection_t pConn,char *buff,ssize_t buflen); 
	void ngx_wait_request_handler_proc_p1(lpngx_connection_t pConn);                                                       
	void ngx_wait_request_handler_proc_plast(lpngx_connection_t pConn);   
	void clearMsgSendQueue();                                             

	ssize_t sendproc(lpngx_connection_t c,char *buff,ssize_t size);      

	//获取对端信息相关                                              
	size_t ngx_sock_ntop(struct sockaddr *sa,int port,u_char *text,size_t len);  

	//连接池 或 连接 相关
	void initconnection();                                               
	void clearconnection();                                               
	lpngx_connection_t ngx_get_connection(int isock);                    
	void ngx_free_connection(lpngx_connection_t pConn);                  
	void inRecyConnectQueue(lpngx_connection_t pConn);                  

	//线程相关函数
	static void* ServerSendQueueThread(void *threadData);                
	static void* ServerRecyConnectionThread(void *threadData);           
	
protected:
	//一些和网络通讯有关的成员变量
	size_t                         m_iLenPkgHeader;                      	
	size_t                         m_iLenMsgHeader;                       
	
private:
	struct ThreadItem   
    {
        pthread_t   _Handle;                                              
        CSocekt     *_pThis;                                            
        bool        ifrunning;                                          

        //构造函数
        ThreadItem(CSocekt *pthis):_pThis(pthis),ifrunning(false){}                             
        //析构函数
        ~ThreadItem(){}        
    };


	int                            m_worker_connections;                
	int                            m_ListenPortCount;                     
	int                            m_epollhandle;                         

	//和连接池有关的
	std::list<lpngx_connection_t>  m_connectionList;                      
	std::list<lpngx_connection_t>  m_freeconnectionList;                  
	std::atomic<int>               m_total_connection_n;                 
	std::atomic<int>               m_free_connection_n;             
	pthread_mutex_t                m_connectionMutex;                     
	pthread_mutex_t                m_recyconnqueueMutex;               
	std::list<lpngx_connection_t>  m_recyconnectionList;              
	std::atomic<int>               m_totol_recyconnection_n;         
	int                            m_RecyConnectionWaitTime;              


	//lpngx_connection_t             m_pfree_connections;               
	
	
	
	std::vector<lpngx_listening_t> m_ListenSocketList;                   
	struct epoll_event             m_events[NGX_MAX_EVENTS];             

	//消息队列
	std::list<char *>              m_MsgSendQueue;                       
	std::atomic<int>               m_iSendMsgQueueCount;                 
	//多线程相关
	std::vector<ThreadItem *>      m_threadVector;                        	
	pthread_mutex_t                m_sendMessageQueueMutex;               
	sem_t                          m_semEventSendQueue;                   
	
};

#endif
