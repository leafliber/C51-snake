#include "public.h"
#include "key.h"
#include "stdio.h"
#include "timer.h"

//定义74HC595控制管脚
sbit SRCLK=P3^6;	//移位寄存器时钟输入
sbit RCLK=P3^5;		//存储寄存器时钟输入
sbit SER=P3^4; 		//串行数据输入

#define LEDDZ_COL_PORT	P0	//点阵列控制端口

u8 death[8]={0x00,0x38,0x6e,0x7c,0x7c,0x6e,0x38,0x00};

u8 gled_col[8]={0x7f,0xbf,0xdf,0xef,0xf7,0xfb,0xfd,0xfe};//LED点阵显示图像的列数据
u8 map[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
u8 step[4][2] = {{1,0},{0,1},{-1,0},{0,-1}};


u8 queue_x[65];
u8 queue_y[65];
u8 leader=0;
u8 tail=0;
u8 length=0;

u8 act=0;
u8 game_over=0;
u8 turn=0;

u8 direction=0; 
u8 head_x = 3;
u8 head_y = 3;

u8 apple_x=4;
u8 apple_y=4;

u8 timer = 0;

u8 i;
u8 j;
/*******************************************************************************
* 函 数 名         : hc595_write_data(u8 dat)
* 函数功能		   : 向74HC595写入一个字节的数据
* 输    入         : dat：数据
* 输    出         : 无
*******************************************************************************/

void display(u8 x, u8 y)
{
	u8 one = 0x01;
	map[x] = map[x] + (one<<y);
}

void del(u8 x, u8 y)
{
	u8 one = 0x01;
	map[x] = map[x] - (one<<y);
}

void push(u8 x, u8 y)
{
	leader++;
	length++;
	leader %= 65;
	queue_x[leader] = x;
	queue_y[leader] = y;
	display(x,y);
}

void pop()
{
	u8 one = 0x01;
	tail++;
	length--;
	tail %= 65;
	del(queue_x[tail], queue_y[tail]);
}



void hc595_write_data(u8 dat)
{
	u8 i=0;
	for(i=0;i<8;i++)//循环8次即可将一个字节写入寄存器中
	{
		SER=dat>>7;//优先传输一个字节中的高位
		dat<<=1;//将低位移动到高位
		SRCLK=0;
		delay_10us(1);
		SRCLK=1;
		delay_10us(1);//移位寄存器时钟上升沿将端口数据送入寄存器中	
	}
	RCLK=0;
	delay_10us(1);
	RCLK=1;//存储寄存器时钟上升沿将前面写入到寄存器的数据输出	
}

void show(u8 gled_row[8])
{
	u8 i=0;
	for(i=0;i<8;i++)//循环8次扫描8行、列
	{
		LEDDZ_COL_PORT=gled_col[i];//传送列选数据
		hc595_write_data(gled_row[i]);//传送行选数据
		delay_10us(100);//延时一段时间，等待显示稳定
		hc595_write_data(0x00);//消影	
	}		
}

void monitor_key()
{
	u8 key=key_scan(0);
	if(key==KEY1_PRESS)
	{
		turn = 1;
	}
	else if(key==KEY2_PRESS)
	{
		turn = 3;
	}
	else if(key==KEY3_PRESS)
	{

	}
	else if(key==KEY4_PRESS)
	{

	}
}

void timer0() interrupt 1
{
	timer0_reset(0x4c,0x00);
	timer++;
	if(timer==15)
	{
		timer = 0;
		act=1;
	}
}

void rand_apple()
{
	u8 k;
	u8 rand=TL0%(64-length);
	u8 count = 0;
	u8 over = 0;
	for(i=0;i<8;i++)
	{
		for(j=0;j<8;j++)
		{
			k = tail;
			over = 0;
			while(k != leader){
				k = (k+1)%65;
				if(i == queue_x[k] && j == queue_y[k])
				{
					over = 1;
				}
			}
			if(over == 0)// over == 0   but not run rightly
			{
				if(count == rand)
				{
					apple_x = i;
					apple_y = j;
					return;
				}
				count++;
			}
		}
	}
}

void main()
{	
	u8 i=0;
	timer0_init(0x01,0x4c,0x00);
	push(head_x,head_y);
	rand_apple();
	display(apple_x, apple_y);
	while(1)
	{
		monitor_key();
		if(act && game_over == 0){
			direction += turn;
			direction %= 4;
			head_x += step[direction][0];
			head_y += step[direction][1];
			if(head_x >=8 || head_x < 0 || head_y >= 8 || head_y < 0)
			{
				game_over = 1;
			}
			
			i = tail;
			while(i != leader){
				i = (i+1)%65;
				if(head_x == queue_x[i] && head_y == queue_y[i])
				{
					game_over = 1;
				}
			}
			
			if(game_over == 0)
			{
				if(head_x == apple_x && head_y == apple_y)
				{
					del(apple_x,apple_y);
					rand_apple();
					display(apple_x,apple_y);
					push(head_x,head_y);
				}
				else
				{
					push(head_x,head_y);
					pop();
				}
			}
			turn = 0;
			act = 0;
		}
		if(game_over)
		{
			show(death);
		}
		else
		{
			show(map);
		}
	}
}