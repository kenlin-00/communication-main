

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ngx_c_conf.h"  //和配置文件处理相关的类,名字带c_表示和类有关
#include "ngx_signal.h"
#include "ngx_func.h"    //各种函数声明

//设置标题有关的全局量
char **g_os_argv;            //原始命令行参数数组,在main中会被赋值
char *gp_envmem = NULL;      //指向自己分配的env环境变量的内存
int  g_environlen = 0;       //环境变量所占内存大小

int main(int argc, char *const *argv)
{             
    g_os_argv = (char **) argv;
    ngx_init_setproctitle();    //把环境变量搬家
    
    //读取配置文件
    CConfig *p_config = CConfig::GetInstance(); //单例类
    if(p_config->Load("nginx.conf") == false) //把配置文件内容载入到内存
    {
        printf("配置文件载入失败，退出!\n");
        exit(1);
    }
 
    //我要保证所有命令行参数我都不 用了
    //ngx_setproctitle("nginx: master process");


    //myconf();
    //mysignal();
    
    for(;;)
    {
        sleep(1); //休息1秒
        printf("休息1秒\n");
    }

    //对于因为设置可执行程序标题导致的环境变量分配的内存，我们应该释放
    if(gp_envmem)
    {
        delete []gp_envmem;
        gp_envmem = NULL;
    }
    printf("程序退出，再见!\n");
    return 0;
}


