
#ifndef __NGX_THREADPOOL_H__
#define __NGX_THREADPOOL_H__

#include <vector>
#include <pthread.h>
#include <atomic>   //c++11里的原子操作

//线程池相关类
class CThreadPool
{
public:
    //构造函数
    CThreadPool();               
    
    //析构函数
    ~CThreadPool();                           

public:
    bool Create(int threadNum);                     
    void StopAll();                                

    void inMsgRecvQueueAndSignal(char *buf);       
    void Call();                                    

private:
    static void* ThreadFunc(void *threadData);        
	//char *outMsgRecvQueue();                       
    void clearMsgRecvQueue();                      

private:
    /
    struct ThreadItem   
    {
        pthread_t   _Handle;                        //线程句柄
        CThreadPool *_pThis;                        //记录线程池的指针	
        bool        ifrunning;                      //标记是否正式启动起来，启动起来后，才允许调用StopAll()来释放

        //构造函数
        ThreadItem(CThreadPool *pthis):_pThis(pthis),ifrunning(false){}                             
        //析构函数
        ~ThreadItem(){}        
    };

private:
    static pthread_mutex_t     m_pthreadMutex;     
    static pthread_cond_t      m_pthreadCond;       
    static bool                m_shutdown;          

    int                        m_iThreadNum;        //要创建的线程数量

    //int                        m_iRunningThreadNum; //线程数, 运行中的线程数	
    std::atomic<int>           m_iRunningThreadNum; 
    time_t                     m_iLastEmgTime;      
    //time_t                     m_iPrintInfoTime;   
    //time_t                     m_iCurrTime;         //当前时间

    std::vector<ThreadItem *>  m_threadVector;      

    //接收消息队列相关
    std::list<char *>          m_MsgRecvQueue;      //接收数据消息队列 
	int                        m_iRecvMsgQueueCount;//收消息队列大小
};

#endif
