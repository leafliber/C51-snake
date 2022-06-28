#include "public.h"
#include "key.h"
#include "stdio.h"
#include "timer.h"

//定义74HC595控制管脚
sbit SRCLK=P3^6;	//移位寄存器时钟输入
sbit RCLK=P3^5;		//存储寄存器时钟输入
sbit SER=P3^4; 		//串行数据输入
sbit BEEP=P1^5;   //音乐
sbit led = P2^0;
#define LEDDZ_COL_PORT	P0	//点阵列控制端口

u8 death[8]={0x00,0x38,0x6e,0x7c,0x7c,0x6e,0x38,0x00};

u8 gled_col[8]={0x7f,0xbf,0xdf,0xef,0xf7,0xfb,0xfd,0xfe};//LED点阵显示图像的列数据
u8 map[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
u8 step[4][2] = {{1,0},{0,1},{-1,0},{0,-1}};

u8 timer_level[3] = {0x4c,0x50,0x60};//难度控制
u8 level = 0;

u8 queue_x[65]; //循环队列x坐标
u8 queue_y[65]; //循环队列y坐标
u8 leader=0; //头指针
u8 tail=0; //尾指针
u8 length=0; //队列长度

u8 act=0; //是否执行
u8 game_over=0; //游戏是否结束
u8 turn=0; //转向方向

u8 direction=0; //当前方向
u8 head_x = 3; //蛇头部节点x轴
u8 head_y = 3; //蛇头部节点y轴

u8 apple_x=4; //食物节点x轴
u8 apple_y=4; //食物节点y轴

u8 timer = 0; //中断次数
u16 sound_long = 0; //声音持续时间

u8 i;
u8 j;
//music
u8 music_index = 0;
u8 soundtone_end[] = {6,5,4,3,2,1,1,2};
u16 soundlong_end[] ={1,1,1,1,1,1,2,1};
u8 freq_h[7] = {0XFC,0XFC,0XFD,0XFD,0XFD,0XFD,0XFE}; //音调 1 2 3 4 5 6 7
u8 freq_l[7] = {0X44,0XAC,0X09,0X34,0X82,0XC8,0X06};
u16 freq[7] = {261.6,293.6,329.6,349.2,392,440,493.8};//对应频率

void play_init() //初始化
{
	TR1 = 0;
	music_index = 0;
	BEEP = 0;
	timer1_reset(0xff,0xff);
}

void play_death() //死亡后执行
{
	TR1=1;
}

void display(u8 x, u8 y) //显示某个点
{
	u8 one = 0x01;
	map[x] = map[x] + (one<<y);
}

void del(u8 x, u8 y) //取消显示某个点
{
	u8 one = 0x01;
	map[x] = map[x] - (one<<y);
}

void push(u8 x, u8 y) //循环队列：入队列
{
	leader++;
	length++;
	leader %= 65;
	queue_x[leader] = x;
	queue_y[leader] = y;
	display(x,y);
}

void pop() //循环队列：出队列
{
	u8 one = 0x01;
	tail++;
	length--;
	tail %= 65;
	del(queue_x[tail], queue_y[tail]);
}


void hc595_write_data(u8 dat)  //数据写一排
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

void show(u8 gled_row[8])  //点阵显示
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

void monitor_key()  //监控按键
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
    	level = (level+1)%2;
	}

}

void timer0() interrupt 1  //定时器0 控制每一步时间间隔
{
	led = 0;
	//timer0_reset(0x4c,0x00);
	timer0_reset(timer_level[level],0x00);
	timer++;
	if(timer==15)
	{
		timer = 0;
		act=1;
	}
}
void timer1() interrupt 3 //定时器1 控制音乐播放速度
{
	BEEP = !BEEP;
	sound_long++;
	timer1_reset(freq_h[soundtone_end[music_index]-1],freq_l[soundtone_end[music_index]-1]);
		if(sound_long > soundlong_end[music_index] * freq[music_index])
		{
			music_index++;
			sound_long = 0;
		}
	if(music_index==6){TR1 = 0;}
}

void rand_apple() //随机生成食物（包括避免生成在自己身上）
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
			if(over == 0)
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
	timer_init();
	play_init();
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
					push(head_x,head_y);
					rand_apple();
					display(apple_x,apple_y);
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
			play_death();
		}
		else
		{
			show(map);
		}
	}
}
