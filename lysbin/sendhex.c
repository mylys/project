/*************************************************
 * Copyright (C), 2013-2015, jfpal., Ltd.
 * File name: sendhex.c
 * Author: lys               
 * Version: 1.0
 * Date:2015/9/9 14:18:37
 * Description: �˺������ڷ���16���Ʊ��ĵ�ָ��ip��port��
 * Others: ����ʹ����Ҫ����ָ���ļ���ʽ�����ݡ�
 * compile��gcc sendhex.c -o sendhex
 * 
*************************************************/
#include <stdio.h>					//��׼�������ͷ�ļ�
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
 *���ܣ� ��ʽ��ascii�ַ���������ַ����е�\t \n \r
 *���룺 char *s        �����ַ���			 
 *����� char *d				��ascii�ַ���
 *����ֵ��
 *����	
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
 *��������	set_setsockopt_function(int server_sockfd ,int type)
 *���ܣ�		����socket����:				SOL_SOCKET
 *���룺		int server_sockfd			��Ҫ���õ�socket
 *				int type							��Ҫ���õ�socket���� 
 					1:�������ñ��ض˿�
 					2:������ʱ�����ӳٹر�Ϊ0�����̹ر�
 					3.������ʱ�����ӳٹر�Ϊ10����ʱ�ر�
 					4.�����Բ��������Ƿ���ڣ��׽��ֱ��
 					5:���ճ�ʱ�����ͳ�ʱʱ���趨����������
 					6:���պͷ��ͻ�������С�趨
 					7:���պͷ��� ��������ˮƽλ���
 	
 	*ʾ��:  ������: 1��2
 					�����ӣ�3,4,5				
 					
 					
 *�����		none
 *����ֵ��	-1			����ʧ��
 *				  0			 ���óɹ�
 ********************************************/

int set_setsockopt_function(int server_sockfd ,int type)
{
		int iRet=0;
		
		int flag=1;
		int len=sizeof(int); 
		
		struct linger short_linger,long_linger;		//���������ã����߳���������                                                               
		short_linger.l_onoff = 0;			//��ʱ״̬���رգ�.C����closesocket()��ǿ�ƹر�                                  
		short_linger.l_linger =0  ;		//���õȴ�ʱ��Ϊ0��,�����ӡ� ��ز�����ӣ�
		long_linger.l_onoff = 10;			//��ʱ״̬���򿪣�.C����closesocket()����ʱ�ر�                                  
		long_linger.l_linger =0  ;		//���õȴ�ʱ��Ϊ0��,�����ӡ� ��ز�����ӣ�


		int keepAlive = 1;// ����keepalive����
		
		int keepIdle = 60; 		//��ʼ�״�KeepAlive̽��ǰ��TCP�ձ�ʱ��;���������60����û���κ���������,�����̽�� 
 		int keepInterval = 5; // ̽��ʱ������ʱ����Ϊ5 ��
  	int keepCount = 3; 		// ̽�Ⳣ�ԵĴ���.�����1��̽������յ���Ӧ��,���2�εĲ��ٷ�.
		
		int rcvbuf_l=1;
		int rcvbuf=4096;
		int sndsize=8192;//��ʵ�Ļ�������СΪ�û����������
		 
		struct timeval rcvtime,sndtime;
		rcvtime.tv_sec=10;//����Ϊ10��
		rcvtime.tv_usec=0;
		sndtime.tv_sec=10;//����Ϊ10��
		sndtime.tv_usec=0;	
		int len1=sizeof(rcvtime);		
		
		int rcvlowat=10;//���ջ���������Ϳ����ֽ���
		int sndlowat=10;//���ͻ���������Ϳ����ֽ�����������	
				
		switch(type)
		{
				case 1:				//����˳����Ӷ����Ӷ�Ҫʹ���׽���ѡ��֪ͨ�ںˣ�����˿�æ����TCP״̬λ�� TIME_WAIT ���������ö˿ڶ����õȴ�60s��
					iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&flag,len);	//SO_REUSEADDRΪҪ���õ�ѡ��������closesocket()һ�㲻�������ر�socket��������TIME_WAIT�Ĺ��̡�BOOL bReuseaddr = TRUE;
					if(iRet<0)
					{
						perror("[ERROR]:setsockopt��SO_REUSEADDR");
						close(server_sockfd);
						return -1;
					}
					return 0;
					
				case 2:				// �������ж�ʱ����Ҫ�ӳٹر�(linger)�Ա�֤�������ݶ������䣬ֱ�ӹر�����socket��
					iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_LINGER,(char *) &short_linger, sizeof(struct linger));
					if (iRet < 0)   
					{                                                                                  
						perror("[ERROR]:setsockopt��SO_LINGER");
						close(server_sockfd);
						return -1;//�������δ�����꣬ȴ������ closesocket()����ʱ����ǿ�ƹرգ����ٷ��ͣ��������ӣ���
					}       
					return 0;
				case 3:				// �������ж�ʱ����Ҫ�ӳٹر�(linger)�Ա�֤�������ݶ������䣬������Ҫ��SO_LINGER���ѡ��(������)//�������δ�����꣬ȴ������ closesocket()����ʱ������ʱ�رգ�������ɺ󣻣������ӣ���
					iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_LINGER,(char *) &long_linger, sizeof(struct linger));
					if (iRet < 0)   
					{                                                                                  
						perror("[ERROR]:setsockopt��SO_LINGER");
						close(server_sockfd);
						return -1;
					}       
					return 0;					


				case 4:				//����������ʹ�ã��ײ��׽��ֱ��ȱ���ⲻ��ʱ���ᱻ����ǽ���ص���������Ʋ����á�
					iRet =setsockopt(server_sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));//����̽��2��Сʱ̽��һ�ζԷ������Ƿ����
 					if (iRet<0)
					{
						fprintf(stderr,"����keepaliveʧ��");
						return -1;
					}
//				//��������ʱ��------��SOL_TCP�������״�̽��ʱ�䣬̽��ʱ������̽��ʱ�����
//					iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));//�״�̽��ǰʱ�䣻
// 				if (iRet<0)
//				{
//					fprintf(stderr,"���� ��ʼ�״�KeepAlive̽��ǰ��TCP�ձ�ʱ�� ʧ��");
//					return -1;
//				}
// 				iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));//�ٴ�̽��ʱ����
// 				if (iRet<0)
//				{
//					fprintf(stderr,"���� ����KeepAlive̽����ʱ���� ʧ��");
//					return -1;
//				}
// 				iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)); //̽�����
// 				if (iRet<0)
//				{
//					fprintf(stderr,"���� �ж��Ͽ�ǰ��KeepAlive̽����� ʧ��");
//					return -1;
//				}	
 					return 0;			
					
				case 5:				//socket�еĳ�ʱ���⣺���ͳ�ʱʱ������ճ�ʱʱ��SO_RCVTIMEO,SO_SNDTIMEOĬ�������recv����û�н��յ����ݻ�һֱ������
					iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_SNDTIMEO,&sndtime,len1);//���ͳ�ʱʱ���趨
					if (iRet < 0)   
					{  
						perror("SO_SNDTIMEO error");
						return -1;
					}
					iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_RCVTIMEO,&rcvtime,len1);//���ճ�ʱʱ���趨
					if (iRet < 0)   
					{ 
						perror("SO_RCVTIMEO");
						return -1;
					}	
					return 0;		
						
 				case 6: 					 //���ý��ܻ�������С  
					if (setsockopt(server_sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf))<0)
					{
							perror("setsockopt:SO_RECVBUF");
							close(server_sockfd);
							return -1;
					}
					//��ȡϵͳ�޸ĺ���ջ�������Сֵ  
       		getsockopt(server_sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, (socklen_t*)&len);  
    			printf("the receive buffer size after setting is %d\n", rcvbuf);  
    			//���÷��ͻ�������С
    			if (setsockopt(server_sockfd, SOL_SOCKET,SO_SNDBUF,&sndsize,sizeof(sndsize))<0) 
    			{
						perror("setsockopt:SO_SNDBUF");
						close(server_sockfd);
						return -1;
 					}
    			
					if (setsockopt(server_sockfd, SOL_SOCKET, SO_RCVLOWAT, &rcvbuf_l, sizeof(rcvbuf))<0) //���յͳ��޶ȣ�Ĭ��Ϊ1�ֽ�
					{
						perror("setsockopt:SO_RCVLOWAT");
						close(server_sockfd);
						return -1;
 					}
 					
// 					if (setsockopt(server_sockfd, SOL_SOCKET, SO_SNDLOWAT, &rcvbuf_l, sizeof(rcvbuf))<0) //���͵ͳ��޶ȣ�Ĭ��Ϊ1�ֽ�
//					{
//						perror("setsockopt: SO_SNDLOWAT");
//						close(server_sockfd);
//						return -1;
// 					}
 					return 0;

 				case 7:	//���ջ�������ˮƽλ���
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
		//������
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
		if(memcmp(data,"[IP]=",5)==0)//ȡ��һ�У�ip
			strcat(sIp,data+5);
		else if(memcmp(data,"[PORT]=",7)==0)//ȡ�ڶ��У�port
		{
				strcat(tmps,data+7);
				lport=atoi(tmps);
		}
		else if(memcmp(data,"[HEX]",5)==0)//ȡ�����У��ַ���ʽ
		{
				hex=1;
		}
		else//ȡʣ�µ�Ҫ���͵��ļ����ݣ���С������2000��
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
   iRet=read_timeout(iSockFd,10);//��ʱʮ���趨
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






