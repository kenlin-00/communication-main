# communication-main


> 本项目是基于 C++ 开发的一套网络通信框架，适合用于客户端和服务器之间保持 TCP 长连接，并且两端之间数据收发不算太频繁的应用场景，比如网络游戏服务器等。

> 项目中大部分实现思路参考了 nginx 官网实现。由于本人也是学习者，因此源码中加了很多注释。仅供自己学习使用

> 在 I/O 多路复用部分 以了一位网友【[王博靖](https://github.com/wangbojing/NtyTcp)】自己写的一套 Epoll 源码为入口，通过阅读源码学习了 Epoll 函数内部的实现原理，并参考其思想引入到本项目中。

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
		- [设置进程名称](#设置进程名称)
	- [日志打印实现](#日志打印实现)
	- [信号功能实现](#信号功能实现)
		- [怎么创建 worker 子进程](#怎么创建-worker-子进程)
	- [守护进程的实现](#守护进程的实现)
	- [关于 Epoll的实现部分](#关于-epoll的实现部分)
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
	//定义一个嵌套列，专门为CConfig服务，还用于释放 m_instance
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

可以通过打印地址发现两个对象答应的地址是一样的

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


### 设置进程名称

**进程名称实际上是保存在 argc[0] 所指向的内存中**。CMD 会把 argv 所指向的命令参数全部显示出来，因为 `./nginx`是保存在 `argv[0]`中，所以 `argv[0]`改变，进程名也就改变了。

> 通过将 环境变量信息 迁移至新的内存，来解决设置的进程名称的长度大于字符串 `./nginx`的长度，可能导致设置的进程名称覆盖其他参数 的问题

修改进程名称在 `ngx_setproctitle()` 函数实现。

**该函数的大致逻辑**

- 计算进程名称的长度

- 计算命令行参数所占内存与环境变量所占内存的总和

- 设置新的进程名称

![](https://cdn.jsdelivr.net/gh/kendall-cpp/blogPic@main/寻offer总结/通信框架-更改运行程序名.1npok0l7nujk.png)

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

> 待更新


## 站在巨人的肩膀上

- 《C++新经典 Linux C++通讯架构实战》

- 《linux内核设计与实现》

- 《linux高性能服务器编程》

- 《Unix网络编程 卷1》

- 《Unix高级环境编程》

- https://github.com/wangbojing/NtyTcp

- https://time.geekbang.org/column/intro/214