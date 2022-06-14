#include "public.h"
#include "key.h"
#include "stdio.h"
#include "timer.h"

//����74HC595���ƹܽ�
sbit SRCLK=P3^6;	//��λ�Ĵ���ʱ������
sbit RCLK=P3^5;		//�洢�Ĵ���ʱ������
sbit SER=P3^4; 		//������������

#define LEDDZ_COL_PORT	P0	//�����п��ƶ˿�

u8 death[8]={0x00,0x38,0x6e,0x7c,0x7c,0x6e,0x38,0x00};

u8 gled_col[8]={0x7f,0xbf,0xdf,0xef,0xf7,0xfb,0xfd,0xfe};//LED������ʾͼ���������
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
* �� �� ��         : hc595_write_data(u8 dat)
* ��������		   : ��74HC595д��һ���ֽڵ�����
* ��    ��         : dat������
* ��    ��         : ��
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
	for(i=0;i<8;i++)//ѭ��8�μ��ɽ�һ���ֽ�д��Ĵ�����
	{
		SER=dat>>7;//���ȴ���һ���ֽ��еĸ�λ
		dat<<=1;//����λ�ƶ�����λ
		SRCLK=0;
		delay_10us(1);
		SRCLK=1;
		delay_10us(1);//��λ�Ĵ���ʱ�������ؽ��˿���������Ĵ�����	
	}
	RCLK=0;
	delay_10us(1);
	RCLK=1;//�洢�Ĵ���ʱ�������ؽ�ǰ��д�뵽�Ĵ������������	
}

void show(u8 gled_row[8])
{
	u8 i=0;
	for(i=0;i<8;i++)//ѭ��8��ɨ��8�С���
	{
		LEDDZ_COL_PORT=gled_col[i];//������ѡ����
		hc595_write_data(gled_row[i]);//������ѡ����
		delay_10us(100);//��ʱһ��ʱ�䣬�ȴ���ʾ�ȶ�
		hc595_write_data(0x00);//��Ӱ	
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