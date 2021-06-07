# communication-main


> 本项目是基于 C++ 开发的一套网络通信框架，适合用于客户端和服务器之间保持 TCP 长连接，并且两端之间数据收发不算太频繁的应用场景，比如网络游戏服务器等。

> 项目中大部分实现思路参考了 nginx 官网实现。由于本人也是学习者，因此源码中加了很多注释。仅供自己学习使用

> 在 I/O 多路复用部分 以了一位网友【[王博靖](https://github.com/wangbojing/NtyTcp)】自己写的一套 Epoll 源码为入口，通过阅读源码学习了 Epoll 函数内部的实现原理，并参考其思想引入到本项目中。

> 下面会将项目完成在过程中的个人详细笔记整理出来，包括代码细节和涉及到的技术原理（更新中...)

------------

<!-- TOC -->

- [communication-main](#communication-main)
	- [配置文件处理](#配置文件处理)
		- [代码中一些要点笔记](#代码中一些要点笔记)
			- [单例模式自动释放，使用嵌套内部类来实现](#单例模式自动释放使用嵌套内部类来实现)
			- [关于 fgets 函数](#关于-fgets-函数)
			- [strlen 与 sizeof 的区别](#strlen-与-sizeof-的区别)
			- [关于 strchr](#关于-strchr)
			- [关于memset](#关于memset)
			- [strcpy 函数和 strncpy 函数的区别](#strcpy-函数和-strncpy-函数的区别)
			- [strcasecmp 函数](#strcasecmp-函数)
	- [内存泄漏检测工具](#内存泄漏检测工具)
	- [设置进程名称](#设置进程名称)
			- [环境变量信息搬家](#环境变量信息搬家)
			- [怎么修改进程名称](#怎么修改进程名称)
		- [代码中一些要点笔记](#代码中一些要点笔记-1)
			- [extern 关键字](#extern-关键字)
			- [`delete`和`delete[]`的区别](#delete和delete的区别)
	- [日志打印实现](#日志打印实现)
	- [信号功能实现](#信号功能实现)
		- [怎么创建 worker 子进程](#怎么创建-worker-子进程)
	- [守护进程的实现](#守护进程的实现)
	- [关于 Epoll的实现部分](#关于-epoll的实现部分)
		- [如何快速从该连接池中找到一个空闲连接分配给套接字](#如何快速从该连接池中找到一个空闲连接分配给套接字)
		- [解决TCP粘包问题](#解决tcp粘包问题)
		- [收包流程](#收包流程)
		- [使用多线程解析和处理数据包](#使用多线程解析和处理数据包)
	- [站在巨人的肩膀上](#站在巨人的肩膀上)

<!-- /TOC -->

------------


## 配置文件处理

读取配置文件各个函数之间的关系图

![](https://cdn.jsdelivr.net/gh/kendall-cpp/blogPic@main/寻offer总结/通信框架-加载配置文件01.1p4vsvly55gg.png)

### 代码中一些要点笔记

####  单例模式自动释放，使用嵌套内部类来实现

```cpp
class CConfig
{
private:
	CConfig();
	static CConfig *m_instance;

public:
	~CConfig();
	static CConfig* GetInstance() {
		if(m_instance == NULL) {
			m_instance = new CConfig();
			//定义一个内部类用于自动释放对象
			static CGarhuishou cl; 
		}
		return m_instance;
	}
	//定义一个嵌套列，专门为CConfig服务，用于释放 m_instance
	class CGarhuishou {
	public:
		~CGarhuishou() {
			if( CConfig::m_instance ) {
				delete m_instance;
				CConfig:m_instance = NULL;
			}
		}
	};

};
```

可以通过打印地址发现两个对象打印的地址是一样的

```cpp
nt main(int argc,char *const *argv) {

    //创建一个读取配置文件的类
    CConfig *p_config = CConfig::GetInstance();
    //  CConfig *p_config1 = CConfig::GetInstance();

    //使用C++的方式打印对象的地址
    cout << "p_config的地址是："  << static_cast<void *>(p_config) << endl;
    cout << "p_config1的地址是："  << static_cast<void *>(p_config1) << endl;
    //使用C语言的方式打印对象的地址
    printf("p_config的地址是：%p\n",p_config);
    printf("p_config1的地址是：%p\n",p_config1);
    //上面输出的地址都是一样的，说明单例模式没问题

    return 0;
}
```

#### 关于 fgets 函数

C 库函数 `char *fgets(char *str, int n, FILE *stream)` 从指定的流 `stream `读取一行，并把它存储在 `str` 所指向的字符串内。当读取` (n-1)` 个字符时，或者读取到换行符时，或者到达文件末尾时，它会停止，具体视情况而定。

> - 虽然用 `gets()` 时有空格也可以直接输入，但是 `gets()` 有一个非常大的缺陷，即它不检查预留存储区是否能够容纳实际输入的数据，换句话说，如果输入的字符数目大于数组的长度，`gets` 无法检测到这个问题，就**会发生内存越界**，所以编程时建议使用 `fgets()`。   
> - `fgets()` 虽然比 `gets()` 安全，但安全是要付出代价的，代价就是它的使用比 `gets()` 要麻烦一点，有三个参数。它的功能是从 `stream` 流中读取 `size` 个字符存储到字符指针变量 `s` 所指向的内存空间。它的返回值是一个指针，指向字符串中**第一个字符的地址**。

#### strlen 与 sizeof 的区别

-  sizeof 是一个单目运算符，strlen是 函数。用 sizeof 时，会在测量的长度后加 `\0` ,而且分别在 int 和 char 的两种情况下得到的结果不同；用 strlen 则是精确算出其长度（不会加`\0`），但是 strlen 读到 `\0` 就会停止。

- 对 sizeof 而言，因为缓冲区已经用已知字符串进行了初始化，其长度是固定的，所以 sizeof 在**编译时**计算缓冲区的长度。也正是由于在编译时计算，因此 sizeof 不能用来返回动态分配的内存空间的大小。

#### 关于 strchr

```c
char * strchr(char * str, char/int c);
```
在字符串 str 中寻找字符`C`第一次出现的位置，并返回其位置（地址指针），若失败则返回NULL；

#### 关于memset

```c
void *memset(void *str, int c, size_t n) 
```
复制字符 c（一个无符号字符）到参数 str 所指向的字符串的前 n 个字符。n 一般都是 c 的长度

#### strcpy 函数和 strncpy 函数的区别

```cpp
char* strcpy(char* strDest, const char* strSrc)
char* strncpy(char* strDest, const char* strSrc, int pos)
```

 `strcpy`函数: 如果参数 `dest` 所指的内存空间不够大，可能会造成缓冲溢出(`buffer Overflow`)的错误情况，在编写程序时请特别留意，或者用`strncpy()`来取代。    
 `strncpy`函数：用来复制源字符串的前`n`个字符，`src` 和 `dest` 所指的内存区域不能重叠，且 `dest` 必须有足够的空间放置`n`个字符。 


#### strcasecmp 函数

```c
int strcasecmp (const char *s1, const char *s2);
```

判断字符串是否相等(忽略大小写),若参数s1 和s2 字符串相同则返回 0。s1 长度大于s2 长度则返回大于 0 的值，s1 长度若小于s2 长度则返回小于 0 的值。

---------

> 以上代码见 tongxin-nginx-01.tar.gz

------------------------------------------

## 内存泄漏检测工具

- Valgrind --> 检查内存管理问题
  - memchaeck --> 用于检查程序运行的时候的内存泄漏

> 需要在 config.mk 中把 DEBUG 开关打开，（可以显示更多信息） `export DEBUG = true`

- memchaeck 的使用

`valgrind --tool=memcheck ./nginx`

可以使用下面命令完全检查内存泄漏

`valgrind --tool=memcheck --leak-check=full ./nginx`

详细显示，可以看到那些发生内存泄漏

`valgrind --tool=memcheck --leak-check=full --show-reachable=yes --trace-children=yes ./nginx`

![](https://cdn.jsdelivr.net/gh/kendall-cpp/blogPic@main/寻offer总结/内存检测工具.42jf6tm2fto0.png)

## 设置进程名称

更改在使用 ps 命令查看进程的时候 CMD 显示的名称，

**最后结果**

` ps -eo pid,ppid,sid,tty,pgrp,comm,stat,cmd | grep -E 'bash|PID|nginx' `

![](https://cdn.jsdelivr.net/gh/kendall-cpp/blogPic@main/寻offer总结/更改进程名称01.2kx4t4wec5m0.png)

**进程名称实际上是保存在 argc[0] 所指向的内存中**。CMD 会把 argv 所指向的命令参数全部显示出来，因为 `./nginx`是保存在 `argv[0]`中，所以 `argv[0]`改变，进程名也就改变了。

> 在这里遇到了个问题，一旦设置的进程名称的长度大雨字符串 `./nginx`的长度，就可能导致设置的进程名称覆盖其他参数。

#### 环境变量信息搬家

由于环境变量信息也是保存在内存中的，并且**保存的位置紧紧邻 argv 所指向的内存**。所以若果设置的进程名称太长，不但会覆盖掉命令行参数，而且很可能覆盖掉环境变量所指向的内容。

为此，借助了 nginx 的源码，想到了一个解决方案，大致思路是：

- **重新分配一块内存**：足够容纳新的 environ 所指向的内容，把 environ 内容搬到这块内存中来。

- 将以往 `argv[0]` 指向的内容替换成实际要修改的新进程名称

> 在参考 nginx 中的一些代码的时候，发现一个问题，有点困惑，就是在 `ngx_init_setproctitle` 函数中有一段` ngx_alloc` 的代码来分配内存，但是没有对应的释放代码

自己写了一个 `ngx_init_setproctitle` 函数，实现了重新分配一块内存，保存 environ 所指向的内存中的内容。大致逻辑如下：

- 统计环境变量的长度（也就是所需要的内存的大小）
  
- 使用 new 来分配所需要大小的内存
  
- 逐个把环境变量的内容复制到这块内存，并让 `environ[i]` （环境变量指针）指向新的内存位置

#### 怎么修改进程名称

编写一个 `ngx_setproctitle()` 函数，但是要注意：

- 要使用命令行参数必须在`ngx_setproctitle` 函数调用之前使用，否则参数会被覆盖

- 设置新的进程的名称的长度不会超过 `原始的命令行参数所占内存 + 环境变量所占内存`

**该函数的大致逻辑**

- 计算进程名称的长度

- 计算命令行参数所占内存与环境变量所占内存的总和

- 设置新的进程名称

![](https://cdn.jsdelivr.net/gh/kendall-cpp/blogPic@main/寻offer总结/通信框架-更改运行程序名.1npok0l7nujk.png)

### 代码中一些要点笔记

#### extern 关键字

如果全局变量不在忘了件的开头定义，作用范围就只是从定义的地方到文件结束，如果在定义这个位置之前的函数引用这个全局变量，那么就应该在引用之前用关键字 extren 对这个变量作“外部变量声明”，表示这个变量是一个已经定义的外部变量。有了这个声明，变量的作用于就可以扩展到 从声明开始到本文件结束。

#### `delete`和`delete[]`的区别

* `delete`只会调用一次析构函数，而`delete[]`会调用每个成员的析构函数

* 用`new`分配的内存用`delete`释放，用`new[]`分配的内存用`delete[]`释放

假如说使用`new int[10]`来开辟一个内存空间，针对这种简单类型，使用`new`分配后不管是数组还是非数组形式释放都是可以的。他们的效果是一样的，**因为分配简单类型内存的时候，内存大小已经确定，系统可以记忆并且进行管理，在析构时，系统不会调用析构函数**。它直接通过指针可以获取实际分配的内存空间，哪怕是一个数组内存空间。


> 以上代码见 tongxin-nginx-02.tar.gz

## 日志打印实现

`void   ngx_log_stderr(int err, const char *fmt, ...);`

- 该函数支持把错误码转换成对应的错误字符串，追加到要显示的字符串末尾

- `ngx_cpymem`: 该函数的功能类似于 `memcpy`,但是 `memcpy` 返回的是指向目标 dst 的指针，而`ngx_cpymem`返回的是目标（复制后的数据）的终点位置，因为有了这个位置后，后续继续复制数据时就很方便了。

- `ngx_vslprintf`: 功能相当于系统的 `printf` 函数

![](https://cdn.jsdelivr.net/gh/kendall-cpp/blogPic@main/寻offer总结/通信框架-日志打印01.69qcjx2373c0.png)

## 信号功能实现

`ngx_init_signals` 函数：用于初始化信号，内部调用系统函数 `sigaction` 来设置信号处理函数。

### 怎么创建 worker 子进程
 
 `ngx_process_cycle.cxx` 文件实现开启子进程，其中`ngx_master_process_cycle()`实现了如何创建 worker 子进程，该函数实现逻辑：

 - 设置 master 进程的进程名称
 - 从配置文件中读取要创建的 worker 进程的数量信息（4个）
 - 接着调用 `ngx_start_worker_processes` 函数创建子进程（4个）
  
> 利用 **for 循环创建多个子进程**，每循环一次就调用一次  `ngx_spawn_process` 函数，`ngx_spawn_process` 中调用 fork 创建子进程，每个子进程分子都会接着调用 `ngx_worker_process_cycle()` 函数。	
> 			
>  `ngx_start_worker_processes` 函数取消对所有信号的屏蔽，为子进程设置标题，利用一个 for 无限循环，保证子进程执行流程一直在这个无限循环中
> 
> 在正常状态下，master 进程会一直在`ngx_master_process_cycle()` 中的 for 无限循环中循环

![](https://cdn.jsdelivr.net/gh/kendall-cpp/blogPic@main/寻offer总结/通信框架-创建worker进程01.30vdiuqy0yk0.png)

在 master 进程和 worker 进程函数的 无限循环代码中增加一条日志输出，**解决日志混乱问题**

## 守护进程的实现

`ngx_deamon.cxx` 文件实现守护进程。`ngx_deamon()` 是核心函数，

> 其中解决了避免子进程编程僵尸进程的问题

## 关于 Epoll的实现部分

源码中 `nty_epoll_rb.c` 和 `nty_epoll_inner.h` 这 2 个文件是 Epoll 相关的 3 个函数的实现文件。

### 如何快速从该连接池中找到一个空闲连接分配给套接字

这里是借鉴了 epoll 的实现思想

- 项目中通过 `CSocekt::ngx_get_connection(int isock)` 和 `CSocekt::ngx_free_connection(lpngx_connection_t pConn)` 这两个函数来实现

### 解决TCP粘包问题

考虑了恶意数据包，畸形数据包问题

具体实现见 `ngx_comm.h`，`ngx_c_socket_conn.cxx`文件


### 收包流程

`CSocket::ngx_write_request_handler()` 函数中

- 首先调用`CSocket::ngx_write_request_handler()`函数将收包初始化以及分配内存

- 利用`CSocket::recvproc`函数来收包（其实开始收的就是包头）
  - 内部其实是调用系统函数 recv 来收包的

- 刚开始是收包头，如果包头收完整了，就调用`CSocket::ngx_wait_request_handler_proc_p1`函数处理该包头，如果包头没收完整，这进入`_PKG_HD_RECVING`状态并继续收包头中剩余的字节。总之，只要包头接收完整，就调用`CSocket::ngx_wait_request_handler_proc_p1`函数来处理。

- 接着开始**处理收到的数据包**
  - 取出整个包的长度放在 e_pkgLen 变量中，根据包的长度判断是否是恶意包并做相应处理，注意，收到一个合法的包头之后，分配足以保存消息头+包头+包体的内存，把消息头、包头率先保存进去。然后继续为后续包体做准备（如果有包体）。如果有一个恶意用户，向服务器只发包头不发包体（甚至发完包头后直接关闭 socket 连接），服务器如何释放刚刚分配的这块内存呢？

  - 项目中在 ngx_connection_t 的结构体中引入了 `ifnewrecvMem`成员，标记新建了一块内存，并用一个指针 `pnewMemPointer`成员指向这块内存。一旦数据包没接收完整，客户端就关闭 `socket`连接，服务器端也能及时回收这块分配出去的内存，以防内存泄漏。

- 包体没收完整，就设置收包状态为 `_PKG_BD_RECVING`并继续收包体；如果包体也收完整了，就调用`CSocket::ngx_wait_request_handler_proc_plast`函数处理整个包。

  - 这里把消息体放进消息队列中，然后重新设置收包状态
  - 为了防止不断向消息队列中放数据导致内存耗费严重，需要适当清理数据，比如当消息数量超过 1000 条时就做清理操作。

到目前为止整个收包流程就完成了。

### 使用多线程解析和处理数据包

将接收的数据包放在接收消息队列中，然后对这些数据包进行解析。具体实现见`ngx_c_socket.cxx`和`ngx_c_socket.h`

## 站在巨人的肩膀上

- 《C++新经典 Linux C++通讯架构实战》

- 《linux内核设计与实现》

- 《linux高性能服务器编程》

- 《Unix网络编程 卷1》

- 《Unix高级环境编程》

- https://github.com/wangbojing/NtyTcp

- https://time.geekbang.org/column/intro/214