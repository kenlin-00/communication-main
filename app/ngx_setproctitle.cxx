//和设置课执行程序标题（名称）相关的放这里 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  //env
#include <string.h>

#include "ngx_global.h"

void ngx_init_setproctitle()
{   
    /
    gp_envmem = new char[g_envneedmem]; 
    memset(gp_envmem,0,g_envneedmem);  //内存要清空防止出现问题

    char *ptmp = gp_envmem;
    //把原来的内存内容搬到新地方来
    for (int i = 0; environ[i]; i++) 
    {
        size_t size = strlen(environ[i])+1 ;
        strcpy(ptmp,environ[i]);    
        environ[i] = ptmp;            
        ptmp += size;
    }
    return;
}

//设置可执行程序标题
void ngx_setproctitle(const char *title)
{

    
    //(1)计算新标题长度
    size_t ititlelen = strlen(title); 

    //(2)计算总的原始的argv那块内存的总长度【包括各种参数】    
    size_t esy = g_argvneedmem + g_envneedmem; //argv和environ内存总和
    if( esy <= ititlelen)
    {
        
        return;
    }

  

    //(3)设置后续的命令行参数为空，表示只有argv[]中只有一个元素了，这是好习惯；防止后续argv被滥用，因为很多判断是用argv[] == NULL来做结束标记判断的;
    g_os_argv[1] = NULL;  

    //(4)把标题弄进来，注意原来的命令行参数都会被覆盖掉，不要再使用这些命令行参数,而且g_os_argv[1]已经被设置为NULL了
    char *ptmp = g_os_argv[0]; //让ptmp指向g_os_argv所指向的内存
    strcpy(ptmp,title);
    ptmp += ititlelen; //跳过标题

    //(5)把剩余的原argv以及environ所占的内存全部清0，否则会出现在ps的cmd列可能还会残余一些没有被覆盖的内容；
    size_t cha = esy - ititlelen;  
    memset(ptmp,0,cha);
    return;
}