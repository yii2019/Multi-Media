#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

//注：以下使用的.bmp文件 可以自行添加,更改对应的文件名即可.
pthread_t Cover,Pic,Video,Game,Music;  //定义事件线程
int finger_fd;  //定义触摸屏fd
int x,y,num;  //定义触摸屏x，y方向，事件号码
struct input_event finger; //定义触摸屏
//pthread_cond_t Cover_cond,Pic_cond,Video_cond,Game_cond,Music_cond;
pthread_cond_t Cover_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t Pic_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t Video_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t Game_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t Music_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER; //初始化互斥锁
/***封面***/
int cover_pic()  //主界面图片处理函数
{
	int cover_bmp_fd = open("cover.bmp",O_RDWR);  //打开图片文件 文件为24位bmp 800*480的图片
	if(cover_bmp_fd < 0)
	{
		perror("cover bmp open ...");
		return -1;
	}
	char head[54];
	read(cover_bmp_fd,head,54);  				//读取头文件数据
	char buf32[480*800*4];
	char buf24[480*800*3];
	read(cover_bmp_fd,buf24,480*800*4);  		//读取像素数据
			                                    
	for(int i=0;i<480*800;i++)					 //24位像素转32位像素
	{
		buf32[0+4*i] = buf24[0+3*i];
		buf32[1+4*i] = buf24[1+3*i];
		buf32[2+4*i] = buf24[2+3*i];
		buf32[3+4*i] = 0;

	}
	char buf_fz[480*800*4];
	for(int n=0;n<480;n++)
	{
		for(int m=0;m<800*4;m++)
		{
			buf_fz[(479-n)*800*4+m] = buf32[(0+n)*800*4+m];    //翻转图片
		}
	}
	close(cover_bmp_fd);
	int lcd_fd = open("/dev/fb0",O_RDWR);  //打开LCD文件
	if(lcd_fd < 0)
		{
			perror("lcd open ...");
			return -1;
		}

	unsigned char *lcd=mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcd_fd,0); //使用虚拟内存

	for(int i=0;i<800*480*4;i++)
		*(lcd+i) = buf_fz[i];
	close(lcd_fd);
	return 0;
}


void *Cover_Fun(void * arg)
{
	while(1)
	{
		pthread_mutex_lock(&mut);
		pthread_cond_wait(&Cover_cond,&mut);
		cover_pic();
		while(1)
		{
			read(finger_fd,&finger,sizeof(finger));   //处理读到的事件值 
			if(finger.type == EV_ABS && finger.code == ABS_X)  //触屏
				x = finger.value;
			if(finger.type == EV_ABS && finger.code == ABS_Y) 
				y = finger.value;

			if(finger.type == EV_KEY && finger.code == BTN_TOUCH && finger.value == 0) 
			{
				//相册按钮	
				if(x>0 && x<99 && y>0 && y<50)
				{
					printf("open picture!\n");
					num = 1;
					usleep(1000);
					break;							
				}
				//视频按钮
				if(x>100 && x<399 && y>0 && y<50)
				{
					printf("open video!\n");
					num = 2;
					usleep(1000);
					break;	
				}

				//游戏按钮
				if(x>400 && x<700 && y>0 && y<50)
				{
					printf("open game!\n");
					num = 3;
					usleep(1000);
					break;	
				}
				//音乐按钮
				if(x>701 && x<800 && y>0 && y<50)
				{
					printf("open music!\n");
					num = 4;
					usleep(1000);
					break;	
				}
				//退出按钮
				if(x>100 && x<700 && y>250 && y<480)
				{
					printf("quit!\n");
					num = -1;
					usleep(8000);
					break;	
				}			
			}
		}
		pthread_mutex_unlock(&mut);
		printf("封面已退出！\n");
		
	}
	pthread_exit(0);
}
/***相册***/
int LCD(char buf_pic[800*480*4])
{
	int lcd_fd = open("/dev/fb0",O_RDWR);  //打开LCD文件
	if(lcd_fd < 0)
		{
			perror("lcd open ...");
			return -1;
		}

	unsigned char *lcd=mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcd_fd,0); //使用虚拟内存

	int i;
	for(i=0;i<800*480*4;i++)
		*(lcd+i) = buf_pic[i];
		close(lcd_fd);
}
char picture_read(char buf_fz[480*800*4],char name[50])
{
	int bmp_fd = open(name,O_RDWR);  //打开图片文件
	if(bmp_fd < 0)
	{
		perror("bmp open ...");
		return -1;
	}
	
	char head[54];
	read(bmp_fd,head,54);  //读取头文件数据

	char buf24[480*800*3];

	char buf32[480*800*4];

	read(bmp_fd,buf24,480*800*3);  //读取像素数据


	int i;                                          //24位像素转32位像素
	for(i=0;i<480*800;i++)
	{
		buf32[0+4*i] = buf24[0+3*i];
		buf32[1+4*i] = buf24[1+3*i];
		buf32[2+4*i] = buf24[2+3*i];
		buf32[3+4*i] = 0;

	}

	int m,n;
	

	for(n=0;n<480;n++)
	{
		for(m=0;m<800*4;m++)
		{
			buf_fz[(479-n)*800*4+m] = buf32[(0+n)*800*4+m];    //翻转图片
		}
	}
	close(bmp_fd);
	return 0;
}

void * Pic_Fun(void * arg)
{
	while(1)
	{
		pthread_mutex_lock(&mut);
		pthread_cond_wait(&Pic_cond,&mut);
		char buf_pic1[480*800*4];
		char buf_pic2[480*800*4];
		char buf_pic3[480*800*4];
		char buf_pic4[480*800*4];
		char buf_pic5[480*800*4];
		char *array[4];
		array[0] = buf_pic1;
		array[1] = buf_pic2;
		array[2] = buf_pic3;
		array[3] = buf_pic4;
		char root1[] = "bmp1.bmp";
		char root2[] = "bmp2.bmp";
		char root3[] = "bmp3.bmp";
		char root4[] = "bmp4.bmp";
		picture_read( buf_pic1,root1);
		picture_read( buf_pic2,root2);
		picture_read( buf_pic3,root3);
		picture_read( buf_pic4,root4);

		LCD(array[0]);  //默认第一张
		int i = 0;
		while(1)
		{	
				
			read(finger_fd,&finger,sizeof(finger));   //处理读到的事件值 
			if(finger.type == EV_ABS && finger.code == ABS_X)  //触屏
					x = finger.value;
			if(finger.type == EV_ABS && finger.code == ABS_Y) 
					y = finger.value;
			
			if(finger.type == EV_ABS && finger.code == ABS_X && (finger.value <= 200)&&(finger.value > 0 )) //左翻页
			{
				usleep(1000);
				if( i == 0)
				{
					LCD(array[3]);
					i = 3;
				}       
				else
				{
					i--;
					LCD(array[i]);
				}                        
			}
			if(finger.type == EV_ABS && finger.code == ABS_X && (finger.value <= 800)&&(finger.value > 600 ))//右翻页
			{
				usleep(1000);
				if(i == 3 )
				{
					LCD(array[0]);
					i = 0;
				}
					
				else
				{
					i++;
					LCD(array[i]);
				}
			}	
			if(finger.type == EV_ABS && finger.code == ABS_X && (finger.value <= 599)&&(finger.value > 201 ))//退出
			{
				num = 0;
				usleep(1000);
				break;
			}			
		}
		pthread_mutex_unlock(&mut);
	}

	pthread_exit(0);
}
/***视频***/
int fifo_fd; 
int Mplayer_Init()  //初始化播放器
{
	if(access("/fifo",F_OK))
	{
		mkfifo("/fifo",0777);//创建管道文件
	}

	fifo_fd = open("/fifo",O_RDWR);
	if(fifo_fd == -1)
		perror("open fifo ...");

	return 0;
}

int Send_Cmd(char * cmd)  //发送命令
{
	write(fifo_fd,cmd,strlen(cmd));

	return 0;
}

void * Video_Fun(void * arg)		//不能控制的话试下子进程运行cmd
{
	while(1)
	{
		pthread_mutex_lock(&mut);
		pthread_cond_wait(&Video_cond,&mut);
		Mplayer_Init();
		system("mplayer -slave -quiet -input file=/fifo -geometry 100:51 -zoom -x 600 -y 430 Faded3.avi & ");
		while(1)
		{		
			read(finger_fd,&finger,sizeof(finger));
			if(finger.type == EV_ABS && finger.code == ABS_X)  //触屏
				x = finger.value;
			if(finger.type == EV_ABS && finger.code == ABS_Y) 
				y = finger.value;

			if(finger.type == EV_KEY && finger.code == BTN_TOUCH && finger.value == 0) //播放器按钮
			{

				
				if(x>701 && x<800 && y>250 && y<480)//快进
				{
					Send_Cmd("seek +10\n");
				}

				if(x>0 && x<99 && y>250 && y<480)//快退
				{
					Send_Cmd("seek -10\n");
				}
				if(x>701 && x<800 && y>51 && y<250)//音量+
				{
					Send_Cmd("volume +10\n");
				}
				if(x>0 && x<99 && y>51 && y<250)//音量-
				{
					Send_Cmd("volume -10\n");
				}
				if(x>100 && x<700 && y>51 && y<250)
				{
					Send_Cmd("pause\n");//暂停代码
				}
				if(x>100 && x<700 && y>250 && y<480)
				{
					num = 0;
					system("killall -15 mplayer");
					usleep(1000);
					break;
				}

			}
		}
		pthread_mutex_unlock(&mut);
	}
	pthread_exit(0);
}
/***游戏***/
int score,lcd;
unsigned int *lcd_mmap;
pthread_t P_id,B_id;
int red = 0x00ff0000, white = 0x00ffffff;
int  plate_y;
int r = 30;
int x0=400;
int y0=240;
int Game_Init()
{	
	x0 = 400;
	y0 = 240;
	printf("start game\n");
	lcd = open("/dev/fb0",O_RDWR);
	if(lcd < 0)
	{
		perror("lcd...");
	}
	lcd_mmap = mmap(NULL,800*480*4,PROT_READ | PROT_WRITE,MAP_SHARED,lcd,0);
	if(lcd_mmap < 0)
	{
		perror("mmap...");
	}

	return 0;
}

void *Draw_Ball(void *arg)			//将屏幕画白
{	
	int  x, y;
	int sign_x=0, sign_y=0;
	for(y=0; y<480; y++)
	{
		for(x=0; x<800; x++)
		{
			*(lcd_mmap+800*y+x) = white;
		}
	}
	while(1)
	{	
		usleep(2000-score*200);
		for(y=y0-31; y<y0+31; y++)
		{						
			for(x=x0-31; x<x0+31; x++)
			{
				if((x-x0)*(x-x0) + (y-y0)*(y-y0) < r*r)
					*(lcd_mmap+800*y+x) = red;			//画红球
				else	
					*(lcd_mmap+800*y+x) = white; 		//其余白色屏幕
			}
		}
		if(y0+r == 478)									//撞到下平面，向上
			{
				sign_y = 0;
				y0--;
			}
		if(x0+r==728 && plate_y-80<y0 && y0<plate_y+80) //撞到挡板，向左
			{
				sign_x = 0;
				score++;
				printf("you get %d score(s)\n",score);
				x0--;
			}
		if(y0-r == 2)									//撞到上面，向下
			{
				sign_y = 1;
				y0++;
			}
		if(x0-r == 2)									//撞到左面，向右
			{
				sign_x = 1;
				x0++;
			}
		if(x0+r == 798) 
			{
				printf("game over, you get %d score(s)！\n",score);
				score=0;
				pthread_cancel(P_id);
				usleep(1000);
				break;
			}
		if(sign_y == 0)	//↑
			y0--;	//y = y -1
		if(sign_x == 0)	//←	
			x0--;	//x = x -1
		if(sign_y == 1) //↓
			y0++; 	//y = y +1
		if(sign_x == 1) //→
			x0++;	//x = x +1

	}
}

int Draw_Plate(int plate_y)		//画挡板
{	
	int x1, y1;
	for(x1=730; x1<745; x1++)
	{	
		for(y1=0; y1<480; y1++)
		{
			if(plate_y-80 < y1 && y1 < plate_y+80)
				*(lcd_mmap + x1 + 800*y1) = red;
			else
				*(lcd_mmap + x1 + 800*y1) = white;
		}
	}
	return 0;
}
void *Game_Ctrl(void *arg)//控制游戏
{
	while(1)
	{
		read(finger_fd,&finger,sizeof(finger));
		if(finger.type == EV_ABS && finger.code == ABS_Y)  plate_y = finger.value;
		Draw_Plate(plate_y);
		
	}
	return NULL;
}

void * Game_Fun(void * arg)
{
	while(1)
	{
		pthread_mutex_lock(&mut);
		pthread_cond_wait(&Game_cond,&mut);
		Game_Init(); //初始化
		int ret = pthread_create(&B_id,NULL,Draw_Ball,NULL);
		if (ret == -1)
		{
			perror("Draw_Ball");
		}
		ret = pthread_create(&P_id,NULL,Game_Ctrl,NULL);
		if (ret == -1)
		{
			perror("Game_Ctrl");
		}
		pthread_join(B_id,NULL);
		pthread_join(P_id,NULL);
		munmap(mmap,800*480*4);
		close(lcd);
		num = 0;
		sleep(1);
		pthread_mutex_unlock(&mut);	
	}

	pthread_exit(0);

}
/***音乐***/
int fifo_music;
int Mplayer_Init_Music()  //初始化播放器   待会记得关 fifo_music
{
	if(access("/fifo_1",F_OK))
	{
		mkfifo("/fifo_1",0777);//创建管道文件
	}

	fifo_music = open("/fifo_1",O_RDWR);
	if(fifo_music == -1)
		perror("open fifo_1 ...");

	return 0;
}
int Send_Cmd_Music(char * cmd)  //发送命令
{
	write(fifo_music,cmd,strlen(cmd));

	return 0;
}

void * Music_Fun(void * arg)
{
	while(1)
	{
		pthread_mutex_lock(&mut);
		pthread_cond_wait(&Music_cond,&mut);
		Mplayer_Init_Music();
		system("mplayer -slave -quiet -input file=/fifo_1 -geometry 100:51 -zoom -x 600 -y 430 faded.mp3 & ");
		while(1)
		{
			read(finger_fd,&finger,sizeof(finger));
			if(finger.type == EV_ABS && finger.code == ABS_X)  //触屏
				x = finger.value;
			if(finger.type == EV_ABS && finger.code == ABS_Y) 
				y = finger.value;

			if(finger.type == EV_KEY && finger.code == BTN_TOUCH && finger.value == 0) //播放器按钮
			{				
				if(x>701 && x<800 && y>250 && y<480)//快进
				{
					Send_Cmd_Music("seek +10\n");
				}
				if(x>0 && x<99 && y>250 && y<480)//快退
				{
					Send_Cmd_Music("seek -10\n");
				}
				if(x>701 && x<800 && y>51 && y<250)//音量+
				{
					Send_Cmd_Music("volume +10\n");
				}
				if(x>0 && x<99 && y>51 && y<250)//音量-
				{
					Send_Cmd_Music("volume -10\n");
				}
				if(x>100 && x<700 && y>51 && y<250)
				{
					Send_Cmd_Music("pause\n");//暂停代码
				}
				if(x>100 && x<700 && y>250 && y<480)
				{
					system("killall -15 mplayer");
					num = 0;
					usleep(1000);
					break;
				}
			}
		}
		pthread_mutex_unlock(&mut);	
	}

	pthread_exit(0);

}
int main()
{
	/***打开触摸屏文件***/
	finger_fd = open("/dev/input/event0",O_RDONLY);
	if (finger_fd == -1)
	{
		perror("finger");
		return -1;
	}

	pthread_create(&Cover,NULL,Cover_Fun,NULL);
	pthread_create(&Pic,NULL,Pic_Fun,NULL);
	pthread_create(&Video,NULL,Video_Fun,NULL);
	pthread_create(&Game,NULL,Game_Fun,NULL);	
	pthread_create(&Music,NULL,Music_Fun,NULL);
	/***1.相册  2.视频 3.游戏 4.音乐***/
	num = 0; //开始num初始化为0，主界面
	while(1)
	{
		switch(num)
		{
			case 0:pthread_cond_signal(&Cover_cond);break;
			case 1:pthread_cond_signal(&Pic_cond);break;
			case 2:pthread_cond_signal(&Video_cond);break;
			case 3:pthread_cond_signal(&Game_cond);break;
			case 4:pthread_cond_signal(&Music_cond);break;
			default:printf("程序已退出！");break;
		}
		if(num == -1)
			break;
	}

	close(finger_fd);
	return 0;
}