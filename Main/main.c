#include "public.h"
#include "key.h"
#include "stdio.h"
#include "timer.h"

//����74HC595���ƹܽ�
sbit SRCLK=P3^6;	//��λ�Ĵ���ʱ������
sbit RCLK=P3^5;		//�洢�Ĵ���ʱ������
sbit SER=P3^4; 		//������������
sbit BEEP=P1^5;   //����
sbit led = P2^0;
#define LEDDZ_COL_PORT	P0	//�����п��ƶ˿�

u8 death[8]={0x00,0x38,0x6e,0x7c,0x7c,0x6e,0x38,0x00};

u8 gled_col[8]={0x7f,0xbf,0xdf,0xef,0xf7,0xfb,0xfd,0xfe};//LED������ʾͼ���������
u8 map[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
u8 step[4][2] = {{1,0},{0,1},{-1,0},{0,-1}};

u8 timer_level[3] = {0x4c,0x50,0x60};//�Ѷȿ���
u8 level = 0;

u8 queue_x[65]; //ѭ������x����
u8 queue_y[65]; //ѭ������y����
u8 leader=0; //ͷָ��
u8 tail=0; //βָ��
u8 length=0; //���г���

u8 act=0; //�Ƿ�ִ��
u8 game_over=0; //��Ϸ�Ƿ����
u8 turn=0; //ת����

u8 direction=0; //��ǰ����
u8 head_x = 3; //��ͷ���ڵ�x��
u8 head_y = 3; //��ͷ���ڵ�y��

u8 apple_x=4; //ʳ��ڵ�x��
u8 apple_y=4; //ʳ��ڵ�y��

u8 timer = 0; //�жϴ���
u16 sound_long = 0; //��������ʱ��

u8 i;
u8 j;
//music
u8 music_index = 0;
u8 soundtone_end[] = {6,5,4,3,2,1,1,2};
u16 soundlong_end[] ={1,1,1,1,1,1,2,1};
u8 freq_h[7] = {0XFC,0XFC,0XFD,0XFD,0XFD,0XFD,0XFE}; //���� 1 2 3 4 5 6 7
u8 freq_l[7] = {0X44,0XAC,0X09,0X34,0X82,0XC8,0X06};
u16 freq[7] = {261.6,293.6,329.6,349.2,392,440,493.8};//��ӦƵ��

void play_init() //��ʼ��
{
	TR1 = 0;
	music_index = 0;
	BEEP = 0;
	timer1_reset(0xff,0xff);
}

void play_death() //������ִ��
{
	TR1=1;
}

void display(u8 x, u8 y) //��ʾĳ����
{
	u8 one = 0x01;
	map[x] = map[x] + (one<<y);
}

void del(u8 x, u8 y) //ȡ����ʾĳ����
{
	u8 one = 0x01;
	map[x] = map[x] - (one<<y);
}

void push(u8 x, u8 y) //ѭ�����У������
{
	leader++;
	length++;
	leader %= 65;
	queue_x[leader] = x;
	queue_y[leader] = y;
	display(x,y);
}

void pop() //ѭ�����У�������
{
	u8 one = 0x01;
	tail++;
	length--;
	tail %= 65;
	del(queue_x[tail], queue_y[tail]);
}


void hc595_write_data(u8 dat)  //����дһ��
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

void show(u8 gled_row[8])  //������ʾ
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

void monitor_key()  //��ذ���
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

void timer0() interrupt 1  //��ʱ��0 ����ÿһ��ʱ����
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
void timer1() interrupt 3 //��ʱ��1 �������ֲ����ٶ�
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

void rand_apple() //�������ʳ����������������Լ����ϣ�
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
