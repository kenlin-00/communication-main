//和日志相关的函数放之类

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>    //uintptr_t
#include <stdarg.h>    //va_start....
#include <unistd.h>    //STDERR_FILENO等
#include <sys/time.h>  //gettimeofday
#include <time.h>      //localtime_r
#include <fcntl.h>     //open
#include <errno.h>     //errno

#include "ngx_global.h"
#include "ngx_macro.h"
#include "ngx_func.h"
#include "ngx_c_conf.h"

//全局量---------------------
//错误等级，和ngx_macro.h里定义的日志等级宏是一一对应关系
static u_char err_levels[][20]  = 
{
    {"stderr"},    //0：控制台错误
    {"emerg"},     //1：紧急
    {"alert"},     //2：警戒
    {"crit"},      //3：严重
    {"error"},     //4：错误
    {"warn"},      //5：警告
    {"notice"},    //6：注意
    {"info"},      //7：信息
    {"debug"}      //8：调试
};
ngx_log_t   ngx_log;

void ngx_log_stderr(int err, const char *fmt, ...)
{    
    va_list args;                        //创建一个va_list类型变量
    u_char  errstr[NGX_MAX_ERROR_STR+1]; //2048  -- ************  +1是我自己填的，感谢官方写法有点小瑕疵，所以动手调整一下
    u_char  *p,*last;

    memset(errstr,0,sizeof(errstr));     
    last = errstr + NGX_MAX_ERROR_STR;          
    p = ngx_cpymem(errstr, "nginx: ", 7);     //p指向"nginx: "之后    
    
    va_start(args, fmt); //使args指向起始的参数
    p = ngx_vslprintf(p,last,fmt,args); //组合出这个字符串保存在errstr里
    va_end(args);        //释放args

    if (err)  //如果错误代码不是0，表示有错误发生
    {
        //错误代码和错误信息也要显示出来
        p = ngx_log_errno(p, last, err);
    }
    
    //若位置不够，那换行也要硬插入到末尾，哪怕覆盖到其他内容
    if (p >= (last - 1))
    {
        p = (last - 1) - 1; 
    }
    *p++ = '\n'; //增加个换行符    

    //往标准错误【一般是屏幕】输出信息    
    write(STDERR_FILENO,errstr,p - errstr); //三章七节讲过，这个叫标准错误，一般指屏幕

    //测试代码：
    //printf("ngx_log_stderr()处理结果=%s\n",errstr);
    //printf("ngx_log_stderr()处理结果=%s",errstr);
    
    return;
}


u_char *ngx_log_errno(u_char *buf, u_char *last, int err)
{
    //以下代码是我自己改造，感觉作者的代码有些瑕疵
    char *perrorinfo = strerror(err); //根据资料不会返回NULL;
    size_t len = strlen(perrorinfo);

    //然后我还要插入一些字符串： (%d:)  
    char leftstr[10] = {0}; 
    sprintf(leftstr," (%d: ",err);
    size_t leftlen = strlen(leftstr);

    char rightstr[] = ") "; 
    size_t rightlen = strlen(rightstr);
    
    size_t extralen = leftlen + rightlen; //左右的额外宽度
    if ((buf + len + extralen) < last)
    {
        //保证整个我装得下，我就装，否则我全部抛弃 ,nginx的做法是 如果位置不够，就硬留出50个位置【哪怕覆盖掉以往的有效内容】，也要硬往后边塞，这样当然也可以；
        buf = ngx_cpymem(buf, leftstr, leftlen);
        buf = ngx_cpymem(buf, perrorinfo, len);
        buf = ngx_cpymem(buf, rightstr, rightlen);
    }
    return buf;
}

void ngx_log_error_core(int level,  int err, const char *fmt, ...)
{
    u_char  *last;
    u_char  errstr[NGX_MAX_ERROR_STR+1];   //这个+1也是我放入进来的，本函数可以参考ngx_log_stderr()函数的写法；

    memset(errstr,0,sizeof(errstr));  
    last = errstr + NGX_MAX_ERROR_STR;   
    
    struct timeval   tv;
    struct tm        tm;
    time_t           sec;   //秒
    u_char           *p;    //指向当前要拷贝数据到其中的内存位置
    va_list          args;

    memset(&tv,0,sizeof(struct timeval));    
    memset(&tm,0,sizeof(struct tm));

    gettimeofday(&tv, NULL);     

    sec = tv.tv_sec;             //秒
    localtime_r(&sec, &tm);      //把参数1的time_t转换为本地时间，保存到参数2中去，带_r的是线程安全的版本，尽量使用
    tm.tm_mon++;                 //月份要调整下正常
    tm.tm_year += 1900;          //年份要调整下才正常
    
    u_char strcurrtime[40]={0};  //先组合出一个当前时间字符串，格式形如：2019/01/08 19:57:11
    ngx_slprintf(strcurrtime,  
                    (u_char *)-1,                       
                    "%4d/%02d/%02d %02d:%02d:%02d",     //格式是 年/月/日 时:分:秒
                    tm.tm_year, tm.tm_mon,
                    tm.tm_mday, tm.tm_hour,
                    tm.tm_min, tm.tm_sec);
    p = ngx_cpymem(errstr,strcurrtime,strlen((const char *)strcurrtime));  
    p = ngx_slprintf(p, last, " [%s] ", err_levels[level]);                
    p = ngx_slprintf(p, last, "%P: ",ngx_pid);                             

    va_start(args, fmt);                     //使args指向起始的参数
    p = ngx_vslprintf(p, last, fmt, args);   //把fmt和args参数弄进去，组合出来这个字符串
    va_end(args);                            //释放args 

    if (err)  //如果错误代码不是0，表示有错误发生
    {
        //错误代码和错误信息也要显示出来
        p = ngx_log_errno(p, last, err);
    }
    //若位置不够，那换行也要硬插入到末尾，哪怕覆盖到其他内容
    if (p >= (last - 1))
    {
        p = (last - 1) - 1; 
    }
    *p++ = '\n'; //增加个换行符       


    ssize_t   n;
    while(1) 
    {        
        if (level > ngx_log.log_level) 
        {

            break;
        }


        //写日志文件        
        n = write(ngx_log.fd,errstr,p - errstr);  //文件写入成功后，如果中途
        if (n == -1) 
        {
            //写失败有问题
            if(errno == ENOSPC) //写失败，且原因是磁盘没空间了
            {

            }
            else
            {
                //这是有其他错误，那么我考虑把这个错误显示到标准错误设备吧；
                if(ngx_log.fd != STDERR_FILENO) //当前是定位到文件的，则条件成立
                {
                    n = write(STDERR_FILENO,errstr,p - errstr);
                }
            }
        }
        break;
    } //end while    
    return;
}


void ngx_log_init()
{
    u_char *plogname = NULL;
    size_t nlen;

    //从配置文件中读取和日志相关的配置信息
    CConfig *p_config = CConfig::GetInstance();
    plogname = (u_char *)p_config->GetString("Log");
    if(plogname == NULL)
    {
        //没读到，就要给个缺省的路径文件名了
        plogname = (u_char *) NGX_ERROR_LOG_PATH; 
    }
    ngx_log.log_level = p_config->GetIntDefault("LogLevel",NGX_LOG_NOTICE);
    //nlen = strlen((const char *)plogname);


    ngx_log.fd = open((const char *)plogname,O_WRONLY|O_APPEND|O_CREAT,0644);  
    if (ngx_log.fd == -1)  //如果有错误，则直接定位到 标准错误上去 
    {
        ngx_log_stderr(errno,"[alert] could not open error log file: open() \"%s\" failed", plogname);
        ngx_log.fd = STDERR_FILENO; //直接定位到标准错误去了        
    } 
    return;
}
