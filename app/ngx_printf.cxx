//和打印格式相关的函数放这里

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>   //类型相关头文件

#include "ngx_global.h"
#include "ngx_macro.h"
#include "ngx_func.h"

//只用于本文件的一些函数声明就放在本文件中
static u_char *ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64,u_char zero, uintptr_t hexadecimal, uintptr_t width);


u_char *ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...) 
{
    va_list   args;
    u_char   *p;

    va_start(args, fmt); //使args指向起始的参数
    p = ngx_vslprintf(buf, last, fmt, args);
    va_end(args);        //释放args   
    return p;
}

u_char *ngx_vslprintf(u_char *buf, u_char *last,const char *fmt,va_list args)
{

    u_char     zero;

    /*
    #ifdef _WIN64
        typedef unsigned __int64  uintptr_t;
    #else
        typedef unsigned int uintptr_t;
    #endif
    */
    uintptr_t  width,sign,hex,frac_width,scale,n;  //临时用到的一些变量

    int64_t    i64;   //保存%d对应的可变参
    uint64_t   ui64;  //保存%ud对应的可变参，临时作为%f可变参的整数部分也是可以的 
    u_char     *p;    //保存%s对应的可变参
    double     f;     //保存%f对应的可变参
    uint64_t   frac;  //%f可变参数,根据%.2f等，取得小数部分的2位后的内容；
    

    while (*fmt && buf < last) //每次处理一个字符，处理的是  "invalid option: \"%s\",%d" 中的字符
    {
        if (*fmt == '%')  //%开头的一般都是需要被可变参数 取代的 
        {
            //-----------------变量初始化工作开始-----------------
            //++fmt是先加后用，也就是fmt先往后走一个字节位置，然后再判断该位置的内容
            zero  = (u_char) ((*++fmt == '0') ? '0' : ' ');  
                                                                
            width = 0;                                       
            sign  = 1;                                      
            hex   = 0;                                       
            frac_width = 0;                               
            i64 = 0;                                      
            ui64 = 0;                                      
            
            //-----------------变量初始化工作结束-----------------


            while (*fmt >= '0' && *fmt <= '9')   
            {
                //第一次 ：width = 1;  第二次 width = 16，所以整个width = 16；
                width = width * 10 + (*fmt++ - '0');
            }

            for ( ;; ) //一些特殊的格式，我们做一些特殊的标记【给一些变量特殊值等等】
            {
                switch (*fmt)  //处理一些%之后的特殊字符
                {
                case 'u':      
                    sign = 0;  
                    fmt++;      
                    continue;   

                case 'X':       
                    hex = 2;    
                    sign = 0;
                    fmt++;
                    continue;
                case 'x':      
                    hex = 1;   
                    sign = 0;
                    fmt++;
                    continue;

                case '.':       
                    fmt++;      
                    while(*fmt >= '0' && *fmt <= '9')  
                    {
                        frac_width = frac_width * 10 + (*fmt++ - '0'); 
                    } //end while(*fmt >= '0' && *fmt <= '9') 
                    break;

                default:
                    break;                
                } //end switch (*fmt) 
                break;
            } //end for ( ;; )

            switch (*fmt) 
            {
            case '%':
                *buf++ = '%';
                fmt++;
                continue;

            case 'd':
                if (sign)  //如果是有符号数
                {
                    i64 = (int64_t) va_arg(args, int);  //va_arg():遍历可变参数，var_arg的第二个参数表示遍历的这个可变的参数的类型
                }
                else //如何是和 %ud配合使用，则本条件就成立
                {
                    ui64 = (uint64_t) va_arg(args, u_int);    
                }
                break;  /

            case 's': //一般用于显示字符串
                p = va_arg(args, u_char *); //va_arg():遍历可变参数，var_arg的第二个参数表示遍历的这个可变的参数的类型

                while (*p && buf < last)  //没遇到字符串结束标记，并且buf值够装得下这个参数
                {
                    *buf++ = *p++;  //那就装，比如  "%s"    ，   "abcdefg"，那abcdefg都被装进来
                }
                
                fmt++;
                continue; //重新从while开始执行 

            case 'P':  //转换一个pid_t类型
                i64 = (int64_t) va_arg(args, pid_t);
                sign = 1;
                break;

            case 'f': 
                f = va_arg(args, double); 
                if (f < 0)  //负数的处理
                {
                    *buf++ = '-'; //单独搞个负号出来
                    f = -f; //那这里f应该是正数了!
                }
                //走到这里保证f肯定 >= 0【不为负数】
                ui64 = (int64_t) f; //正整数部分给到ui64里
                frac = 0;

                //如果要求小数点后显示多少位小数
                if (frac_width) //如果是%d.2f，那么frac_width就会是这里的2
                {
                    scale = 1;  //缩放从1开始
                    for (n = frac_width; n; n--) 
                    {
                        scale *= 10; //这可能溢出哦
                    }

                    frac = (uint64_t) ((f - (double) ui64) * scale + 0.5);   /

                    if (frac == scale)   
                    {
                        ui64++;    //正整数部分进位
                        frac = 0;  //小数部分归0
                    }
                } //end if (frac_width)

                //正整数部分，先显示出来
                buf = ngx_sprintf_num(buf, last, ui64, zero, 0, width); //把一个数字 比如“1234567”弄到buffer中显示

                if (frac_width) //指定了显示多少位小数
                {
                    if (buf < last) 
                    {
                        *buf++ = '.'; //因为指定显示多少位小数，先把小数点增加进来
                    }
                    buf = ngx_sprintf_num(buf, last, frac, '0', 0, frac_width); //frac这里是小数部分，显示出来，不够的，前边填充'0'字符
                }
                fmt++;
                continue;  //重新从while开始执行

            //..................................
            //................其他格式符，逐步完善
            //..................................

            default:
                *buf++ = *fmt++; //往下移动一个字符
                continue; 
            } //end switch (*fmt) 
            
            //显示%d的，会走下来，其他走下来的格式日后逐步完善......

            //统一把显示的数字都保存到 ui64 里去；
            if (sign) //显示的是有符号数
            {
                if (i64 < 0)  //这可能是和%d格式对应的要显示的数字
                {
                    *buf++ = '-';  //小于0，自然要把负号先显示出来
                    ui64 = (uint64_t) -i64; //变成无符号数（正数）
                }
                else //显示正数
                {
                    ui64 = (uint64_t) i64;
                }
            } //end if (sign) 

            //把一个数字 比如“1234567”弄到buffer中显示，如果是要求10位，则前边会填充3个空格比如“   1234567”
            //注意第5个参数hex，是否以16进制显示，比如如果你是想以16进制显示一个数字则可以%Xd或者%xd，此时hex = 2或者1
            buf = ngx_sprintf_num(buf, last, ui64, zero, hex, width); 
            fmt++;
        }
        else  //当成正常字符，源【fmt】拷贝到目标【buf】里
        {
            //用fmt当前指向的字符赋给buf当前指向的位置，然后buf往前走一个字符位置，fmt当前走一个字符位置
            *buf++ = *fmt++;   //*和++优先级相同，结合性从右到左，所以先求的是buf++以及fmt++，但++是先用后加；
        } //end if (*fmt == '%') 
    }  //end while (*fmt && buf < last) 
    
    return buf;
}


static u_char * ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero, uintptr_t hexadecimal, uintptr_t width)
{
    //temp[21]
    u_char      *p, temp[NGX_INT64_LEN + 1];   //#define NGX_INT64_LEN   (sizeof("-9223372036854775808") - 1)     = 20   ，注意这里是sizeof是包括末尾的\0，不是strlen；             
    size_t      len;
    uint32_t    ui32;

    static u_char   hex[] = "0123456789abcdef";  //跟把一个10进制数显示成16进制有关，换句话说和  %xd格式符有关，显示的16进制数中a-f小写
    static u_char   HEX[] = "0123456789ABCDEF";  //跟把一个10进制数显示成16进制有关，换句话说和  %Xd格式符有关，显示的16进制数中A-F大写

    p = temp + NGX_INT64_LEN; //NGX_INT64_LEN = 20,所以 p指向的是temp[20]那个位置，也就是数组最后一个元素位置

    if (hexadecimal == 0)  
    {
        if (ui64 <= (uint64_t) NGX_MAX_UINT32_VALUE)   //NGX_MAX_UINT32_VALUE :最大的32位无符号数：十进制是‭4294967295‬
        {
            ui32 = (uint32_t) ui64; 
            {
                *--p = (u_char) (ui32 % 10 + '0');  
            }
            while (ui32 /= 10); //每次缩小10倍等于去掉屁股后边这个数字
        }
        else
        {
            do 
            {
                *--p = (u_char) (ui64 % 10 + '0');
            } while (ui64 /= 10); //每次缩小10倍等于去掉屁股后边这个数字
        }
    }
    else if (hexadecimal == 1)  
    {
        do 
        {            
            *--p = hex[(uint32_t) (ui64 & 0xf)];    
        } while (ui64 >>= 4);   
    } 
    else // hexadecimal == 2   
    { 
       
        do 
        { 
            *--p = HEX[(uint32_t) (ui64 & 0xf)];
        } while (ui64 >>= 4);
    }

    len = (temp + NGX_INT64_LEN) - p;  //得到这个数字的宽度，比如 “7654321”这个数字 ,len = 7

    while (len++ < width && buf < last)  
    {
        *buf++ = zero; 
    }
    
    len = (temp + NGX_INT64_LEN) - p; 


    if((buf + len) >= last)   
    {
        len = last - buf; //剩余的buf有多少我就拷贝多少
    }

    return ngx_cpymem(buf, p, len); //把最新buf返回去；
}

