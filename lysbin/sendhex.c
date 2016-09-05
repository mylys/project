/*************************************************
 * Copyright (C), 2013-2015, jfpal., Ltd.
 * File name: sendhex.c
 * Author: lys               
 * Version: 1.0
 * Date:2015/9/9 14:18:37
 * Description: 此函数用于发送16进制报文到指定ip，port。
 * Others: 程序使用需要传参指定文件格式的内容。
 * compile：gcc sendhex.c -o sendhex
 * 
*************************************************/
#include <stdio.h>					//标准输入输出头文件
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <termio.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <signal.h>
#include <netdb.h>
#include <dirent.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <iconv.h>
#include <termios.h>


/***********************************************
 *功能： 格式化ascii字符串，清楚字符串中的\t \n \r
 *输入： char *s        整个字符串			 
 *输出： char *d				纯ascii字符串
 *返回值：
 *例：	
 **********************************************/
int formatstrasc(char *s,char *d)
{
	int ilen,i;
	char *sp1,*sp2;
	
	ilen=strlen(s);
	sp1=s;
	sp2=d;
	for(i=0;i<ilen;i++)
	{
		if(*sp1==' '||*sp1=='\t'||*sp1=='\n'||*sp1=='\r') 
		{
			sp1++;continue;
		}
		memcpy(sp2,sp1,1);
		sp1++;
		sp2++;
	}
	return strlen(d);
}

static void sock_enable_linger(int s)
{
       struct linger li;
			 li.l_onoff = 0;     /*C*/
       //li.l_onoff = 1;     /*java*/
       li.l_linger =0  ;

       if (setsockopt(s, SOL_SOCKET, SO_LINGER,
                      (char *) &li, sizeof(struct linger)) < 0) {
            //  gen_debug2("setsockopt(SO_LINGER) FAIL:%s",strerror(errno));
	}
}


/********************************************
 *函数名：	set_setsockopt_function(int server_sockfd ,int type)
 *功能：		设置socket属性:				SOL_SOCKET
 *输入：		int server_sockfd			需要设置的socket
 *				int type							需要设置的socket类型 
 					1:允许重用本地端口
 					2:短连接时设置延迟关闭为0，立刻关闭
 					3.长连接时设置延迟关闭为10，延时关闭
 					4.周期性测试连接是否存在，套接字保活。
 					5:接收超时，发送超时时间设定。避免阻塞
 					6:接收和发送缓冲区大小设定
 					7:接收和发送 缓冲区低水平位标记
 	
 	*示例:  短连接: 1，2
 					长连接：3,4,5				
 					
 					
 *输出：		none
 *返回值：	-1			设置失败
 *				  0			 设置成功
 ********************************************/

int set_setsockopt_function(int server_sockfd ,int type)
{
		int iRet=0;
		
		int flag=1;
		int len=sizeof(int); 
		
		struct linger short_linger,long_linger;		//短连接设置，或者长连接设置                                                               
		short_linger.l_onoff = 0;			//延时状态（关闭）.C调用closesocket()后强制关闭                                  
		short_linger.l_linger =0  ;		//设置等待时间为0秒,短连接。 （夭折连接）
		long_linger.l_onoff = 10;			//延时状态（打开）.C调用closesocket()后延时关闭                                  
		long_linger.l_linger =0  ;		//设置等待时间为0秒,短连接。 （夭折连接）


		int keepAlive = 1;// 开启keepalive属性
		
		int keepIdle = 60; 		//开始首次KeepAlive探测前的TCP空闭时间;如该连接在60秒内没有任何数据往来,则进行探测 
 		int keepInterval = 5; // 探测时发包的时间间隔为5 秒
  	int keepCount = 3; 		// 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
		
		int rcvbuf_l=1;
		int rcvbuf=4096;
		int sndsize=8192;//真实的缓冲区大小为用户输入的两倍
		 
		struct timeval rcvtime,sndtime;
		rcvtime.tv_sec=10;//设置为10秒
		rcvtime.tv_usec=0;
		sndtime.tv_sec=10;//设置为10秒
		sndtime.tv_usec=0;	
		int len1=sizeof(rcvtime);		
		
		int rcvlowat=10;//接收缓冲区的最低空闲字节数
		int sndlowat=10;//发送缓冲区的最低空闲字节数不能设置	
				
		switch(type)
		{
				case 1:				//服务端长连接短连接都要使用套接字选项通知内核，如果端口忙，但TCP状态位于 TIME_WAIT ，可以重用端口而不用等待60s。
					iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&flag,len);	//SO_REUSEADDR为要设置的选项名调用closesocket()一般不会立即关闭socket，而经历TIME_WAIT的过程。BOOL bReuseaddr = TRUE;
					if(iRet<0)
					{
						perror("[ERROR]:setsockopt：SO_REUSEADDR");
						close(server_sockfd);
						return -1;
					}
					return 0;
					
				case 2:				// 当连接中断时，需要延迟关闭(linger)以保证所有数据都被传输，直接关闭数据socket。
					iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_LINGER,(char *) &short_linger, sizeof(struct linger));
					if (iRet < 0)   
					{                                                                                  
						perror("[ERROR]:setsockopt：SO_LINGER");
						close(server_sockfd);
						return -1;//针对数据未发送完，却调用了 closesocket()；此时做法强制关闭，不再发送；（短连接）。
					}       
					return 0;
				case 3:				// 当连接中断时，需要延迟关闭(linger)以保证所有数据都被传输，所以需要打开SO_LINGER这个选项(长连接)//针对数据未发送完，却调用了 closesocket()；此时做法延时关闭，发送完成后；（长连接）。
					iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_LINGER,(char *) &long_linger, sizeof(struct linger));
					if (iRet < 0)   
					{                                                                                  
						perror("[ERROR]:setsockopt：SO_LINGER");
						close(server_sockfd);
						return -1;
					}       
					return 0;					


				case 4:				//长连接设置使用：底层套接字保活。缺点检测不及时，会被防火墙拦截掉，保活机制不好用。
					iRet =setsockopt(server_sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));//开启探测2个小时探测一次对方主机是否存在
 					if (iRet<0)
					{
						fprintf(stderr,"设置keepalive失败");
						return -1;
					}
//				//重新设置时间------用SOL_TCP来设置首次探测时间，探测时间间隔，探测时间次数
//					iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));//首次探测前时间；
// 				if (iRet<0)
//				{
//					fprintf(stderr,"设置 开始首次KeepAlive探测前的TCP空闭时间 失败");
//					return -1;
//				}
// 				iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));//再次探测时间间隔
// 				if (iRet<0)
//				{
//					fprintf(stderr,"设置 两次KeepAlive探测间的时间间隔 失败");
//					return -1;
//				}
// 				iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)); //探测次数
// 				if (iRet<0)
//				{
//					fprintf(stderr,"设置 判定断开前的KeepAlive探测次数 失败");
//					return -1;
//				}	
 					return 0;			
					
				case 5:				//socket中的超时问题：发送超时时延与接收超时时延SO_RCVTIMEO,SO_SNDTIMEO默认情况下recv函数没有接收到数据会一直阻塞的
					iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_SNDTIMEO,&sndtime,len1);//发送超时时间设定
					if (iRet < 0)   
					{  
						perror("SO_SNDTIMEO error");
						return -1;
					}
					iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_RCVTIMEO,&rcvtime,len1);//接收超时时间设定
					if (iRet < 0)   
					{ 
						perror("SO_RCVTIMEO");
						return -1;
					}	
					return 0;		
						
 				case 6: 					 //设置接受缓冲区大小  
					if (setsockopt(server_sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf))<0)
					{
							perror("setsockopt:SO_RECVBUF");
							close(server_sockfd);
							return -1;
					}
					//获取系统修改后接收缓冲区大小值  
       		getsockopt(server_sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, (socklen_t*)&len);  
    			printf("the receive buffer size after setting is %d\n", rcvbuf);  
    			//设置发送缓冲区大小
    			if (setsockopt(server_sockfd, SOL_SOCKET,SO_SNDBUF,&sndsize,sizeof(sndsize))<0) 
    			{
						perror("setsockopt:SO_SNDBUF");
						close(server_sockfd);
						return -1;
 					}
    			
					if (setsockopt(server_sockfd, SOL_SOCKET, SO_RCVLOWAT, &rcvbuf_l, sizeof(rcvbuf))<0) //接收低潮限度：默认为1字节
					{
						perror("setsockopt:SO_RCVLOWAT");
						close(server_sockfd);
						return -1;
 					}
 					
// 					if (setsockopt(server_sockfd, SOL_SOCKET, SO_SNDLOWAT, &rcvbuf_l, sizeof(rcvbuf))<0) //发送低潮限度：默认为1字节
//					{
//						perror("setsockopt: SO_SNDLOWAT");
//						close(server_sockfd);
//						return -1;
// 					}
 					return 0;

 				case 7:	//接收缓冲区低水平位标记
 					iRet =setsockopt(server_sockfd,SOL_SOCKET,SO_RCVLOWAT,&rcvlowat,sizeof(rcvlowat));
					if (iRet<0)
					{
						perror("SO_RCVLOWAT ERROR");
						return -1;
					}
					iRet =setsockopt(server_sockfd,SOL_SOCKET,SO_SNDLOWAT,&sndlowat,sizeof(sndlowat));
					if (iRet<0)
					{
						perror("SO_SNDLOWAT ERROR");
						return -1;
					}
					return 0;
		}
}	


int INET_Connect(int port,char *address)
{
	int sockfd=0;
	int iRet,n;
	struct sockaddr_in srv_addr;
	int rcvbuf_l=1;
	int rcvbuf=4096;

	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<=0)
	{
		perror("SOCKET ERROR");
		return(-1);
	}
	memset(&srv_addr,0,sizeof(struct sockaddr_in));
		//短连接
	iRet=set_setsockopt_function(sockfd ,1);
	iRet=set_setsockopt_function(sockfd ,2);                             
	if(iRet<0)                     
	{                              
		perror("[ERROR]:setsockopt-et_setsockopt_function");
		close(sockfd);        
		return -1;                   
	} 

	srv_addr.sin_family=AF_INET;
	srv_addr.sin_port=htons(port);
	srv_addr.sin_addr.s_addr=inet_addr(address);

	if(connect(sockfd,(struct sockaddr *)&srv_addr,sizeof(struct sockaddr_in)	)<0)
	{
		perror("Connect Error");
		close(sockfd);
		return(-1);
	}
	return(sockfd);
}

unsigned char Conv(unsigned char Oct)
{
    if (Oct >= 0x41) return ( (Oct - 7) & 0x0F );
    else             return ( Oct & 0x0F );
}

void *asc_hex(unsigned char * Ptd ,int Ld,unsigned char *Pts,int Ls)
{
		int I;

    (void) memset( Ptd, 0x00, Ld ) ;

    Ptd = Ptd + Ld - ((Ls + 1) / 2) ;
    if ( Ls % 2 ) 
    	*Ptd++ = Conv(*Pts++) & 0x0F ;
    for ( I = 0 ; I < (Ls / 2) ; I++)
    {
			*Ptd =  (Conv(*Pts++) << 4) & 0xF0 ;
    	*Ptd++ = *Ptd + (Conv(*Pts++) & 0x0F)  ;
    }
    return((unsigned char*)Ptd);
}

void DisplayCharData(char *ptrHead,char *ptrSource,int iSourceLen)
{
	int i;
	unsigned char tmp[1024];
	unsigned char buf[20];
	
	if(iSourceLen < 0 || iSourceLen > 1024)
	iSourceLen = 1024 ;
	
	memset(buf,0,sizeof(buf));
	memset(tmp,0,sizeof(tmp));
	memcpy(tmp,ptrSource,iSourceLen);
	fprintf(stderr,"%s",ptrHead);
	for(i=0;i<iSourceLen;i++)
	{
//	 	fprintf(stderr,"[%02x]",tmp[i]&0xff);
	 	fprintf(stderr,"%02x ",tmp[i]&0xff);
	  if(i>0 && !((i+1)%15) && (i+1)<iSourceLen)
	  	fprintf(stderr,"\n");
	}
	fprintf(stderr,"\n");
}

int read_timeout(int fd, int timeout)
{
	fd_set rset;
	struct timeval tv;
	
	bzero( &rset, sizeof(fd_set) );//memset(&ret,0,sizeof(ret)); 
	
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	//printf("int wait %d\n",timeout);
	return ( select(fd+1, &rset, NULL, NULL, &tv) );	
}

int main(int argc,char **argv)
{
	int ilen,iRet,iSockFd,i;
	char tmp[2048];
	char hextmp[4096];
	char *sp;
	char	tmps[8];
	char	data[200],sIp[18];
	char 	sFileName[200];
	char	strFromFile[200];
	FILE	*fp;
	long	file_size;
	int lport;
	int hex;
	
	memset(sFileName,0x00,sizeof(sFileName));
	memset(hextmp,0x00,sizeof(hextmp));
	memset(sIp,0x00,sizeof(sIp));
	memset(tmps,0x00,sizeof(tmps));
	
	if(argc!=2)
	{
			printf("usage :%s a.txt\n",argv[0]);
			return 0;	
	}
	ilen=strlen(argv[1]);
	if(ilen==0||ilen>150)
	{
			printf("wrong input filename\n");
			return 0;
	}
	strcat(sFileName,argv[1]);
//	printf("[%s]filename\n",sFileName);
	if( (fp = fopen(sFileName,"r"))==NULL )
	{
			printf("cant open [%s]\n",sFileName);
			return 0;
	}
	memset(tmp,0x00,2048);
	hex=0;
	
	while(!feof(fp))
	{
		memset(strFromFile,0x00,sizeof(strFromFile));
		memset(data,0x00,sizeof(data));
		fgets(strFromFile,sizeof(strFromFile),fp);
		ilen=formatstrasc(strFromFile,data);
		if(memcmp(data,"[IP]=",5)==0)//取第一行，ip
			strcat(sIp,data+5);
		else if(memcmp(data,"[PORT]=",7)==0)//取第二行，port
		{
				strcat(tmps,data+7);
				lport=atoi(tmps);
		}
		else if(memcmp(data,"[HEX]",5)==0)//取第三行，字符格式
		{
				hex=1;
		}
		else//取剩下的要发送的文件内容，大小不超过2000个
		{
				ilen=strlen(tmp);
				if(ilen+strlen(data)>2000)
					{
						printf("cant send packet len over 2000[%d+%d>2000]",ilen,strlen(data));
						return 0;
					}
				strcat(tmp,data);
		}
	}
	fclose(fp);
	if(lport==0||strlen(sIp)==0)
	{
			printf("not set ip or port,please check it\n");
			return 0;
	}
	
	printf("-----connect[ %s %d ]\n",sIp,lport);
	iSockFd=INET_Connect(lport,sIp);
	if(iSockFd<0) 
	{
		printf("Connect error\n");
		return 0;
	}
	
	if(hex)
	{
		ilen=strlen(tmp);
		asc_hex(hextmp,ilen/2,tmp,ilen);
	}
	
	ilen=strlen(tmp);
	if(hex)
	{
		DisplayCharData("-----send hex:\n",hextmp,ilen/2);
		iRet=write(iSockFd,hextmp,ilen/2);
	}
	else
	{
		printf("send[%s]\n",tmp);
		iRet=write(iSockFd,tmp,ilen);
	}	
	if(iRet<=0)
	{
		printf("-----write end [%d]!\n",iRet);
  	close(iSockFd);
		return 0;
	}	
	
//	sleep(50);
   iRet=read_timeout(iSockFd,10);//超时十秒设定
	if(iRet<=0)
	{
		printf("-----read_timeout 50\n");
		return 0;
	}
	memset(tmp,0x00,1024);
	i=0;ilen=0;
	i=read(iSockFd,tmp,1024);ilen+=i;
//	i=read(iSockFd,tmp+i,1024);ilen+=i;
	//if(hex)
	//{
		DisplayCharData("-----read hex:\n",tmp,ilen);
	//}
	if(ilen<=0)
	{
		printf("read error\n");
	}
	close(iSockFd);
	printf("read[%s][%d][%d]\n",tmp,ilen,hex);
	return 0;
}






