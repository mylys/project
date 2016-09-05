/************���ڱ��ĵ�********************************************
 * *filename: epoll-ET-server.c
 * *purpose: ��ʾepoll������socket���ӵķ���
 * *Linux������ Linux֪ʶ������ SOHO�� ������ ���ó�C����
 * *date time:2007-01-31 21:00
 * *Note: �κ��˿������⸴�ƴ��벢������Щ�ĵ�����Ȼ���������ҵ��;
 * *������ѭGPL
 * *Thanks to:Google
 * *Hope:ϣ��Խ��Խ����˹����Լ���������Ϊ��ѧ������չ����
 * *�Ƽ�վ�ھ��˵ļ���Ͻ������죡��л�п�Դǰ���Ĺ��ף�
 * *********************************************************************/

#include "epoll-ET-server.h"



/*
 * setNonBlocking - ���þ��Ϊ��������ʽ
 * */
void setNonBlocking(int sock)                                                                                  
{                                                                                                              
    int opts;                                                                                                  
    opts=fcntl(sock,F_GETFL);                                                                                  
    if(opts<0)                                                                                                 
    {                                                                                                          
        perror("fcntl(sock,GETFL)");                                                                           
        exit(1);                                                                                               
    }                                                                                                          
    opts = opts|O_NONBLOCK;                                                                                    
    if(fcntl(sock,F_SETFL,opts)<0)                                                                             
    {                                                                                                          
        perror("fcntl(sock,SETFL,opts)");                                                                      
        exit(1);                                                                                               
    }                                                                                                          
}



/*
 * handle_message - ����ÿ�� socket �ϵ���Ϣ�շ�
 * */
int handle_message(int new_fd)
{
  char buf[1024],sumbuf[MAXBUF];
  int buflen,sumlen=0,readflag=1;

  bzero(buf, sizeof(buf));
  bzero(sumbuf, sizeof(sumbuf));

  while(readflag)           //ETģ���ر�ע���д
  {
    buflen = recv(new_fd, buf, sizeof(buf), 0);
    if(buflen < 0)
    {
      //�����Ƿ�������ģʽ,���Ե�errnoΪEAGAINʱ,��ʾ��ǰ�������������ݿɶ�
      // ������͵����Ǹô��¼��Ѵ���.
      if(errno== EAGAIN || errno == EINTR || errno == EWOULDBLOCK)  //����buflen<0��errno=EAGAINʱ����ʾû�������ˡ�(��/д��������)
      {
        printf("û�����ݶ�д��\n");
        break;
      }
      else
      {
        printf("��Ϣ����ʧ�ܣ����������%d��������Ϣ��'%s'\n",errno, strerror(errno));
        close(new_fd);
        return -1;                 //���ʧ���ˡ�
      }
    }
    else if(buflen == 0)//�����ʾ�Զ˵�socket�������ر�.
    {
      printf("�Զ˵�socket�������ر�\n");
      return 0;
    }
    else
    {
      //printf("%d������Ϣ�ɹ�:'%s'����%d���ֽڵ�����\n",new_fd, buf, buflen);
      memcpy(sumbuf+sumlen,buf,buflen);
      sumlen+=buflen;
      bzero(buf, sizeof(buf));
    }

    if(buflen == sizeof(buf))
      readflag = 1;   //��Ҫ�ٴζ�ȡ(�п�������Ϊ���ݻ�����buf̫С����������û�ж���)
    else
      readflag =0;    //����Ҫ�ٴζ�ȡ(��buflen
  }
  printf("---------socket[%d]������Ϣ�ɹ�LEN[%d]-[%s]\n",new_fd, sumlen, sumbuf);
   /* ����ÿ���������ϵ������շ����� */
   return sumlen;
}

// �����߳�  
void ListenThreadProcess( void* sEpollServer )  
{  
    EpollServer *pEpollServer = (EpollServer*)sEpollServer;
    sockaddr_in client_addr;  
    int len = sizeof (client_addr);  
    while ( 1 )  
    {  
        int new_socket = accept (pEpollServer->server_sockfd, ( sockaddr * ) &client_addr,(socklen_t*)&len );  
        if ( new_socket < 0 )  
        {  
            printf("Server Acceptʧ��!, client_socket: %d\n", client_socket);  
            continue;  
        }  
        else
        {  
            struct epoll_event    ev;  
            ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;  
            ev.data.fd = new_socket;     //��¼socket���  
            epoll_ctl(pEpollServer->m_iEpollFd, EPOLL_CTL_ADD, new_socket, &ev);  
        }  
    }
    
    while ( 1 )  
    {  
        struct epoll_event    events[_MAX_SOCKFD_COUNT];  
        int nfds = epoll_wait( m_iEpollFd, events,  _MAX_SOCKFD_COUNT, -1 );  
        for (int i = 0; i < nfds; i++)  
        {  
            int client_socket = events[i].data.fd;  
            char buffer[1024];//ÿ���շ����ֽ���С��1024�ֽ�  
            memset(buffer, 0, 1024);  
            if (events[i].events & EPOLLIN)//���������¼�����������  
            {  
                int rev_size = recv(events[i].data.fd,buffer, 1024,0);  
                if( rev_size <= 0 )  
                {  
                    cout << "recv error: recv size: " << rev_size << endl;  
                    struct epoll_event event_del;  
                    event_del.data.fd = events[i].data.fd;  
                    event_del.events = 0;  
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, event_del.data.fd, &event_del);  
                }  
                else  
                {  
                    printf("Terminal Received Msg Content:%s\n",buffer);  
                    struct epoll_event    ev;  
                    ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP;  
                    ev.data.fd = client_socket;     //��¼socket���  
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, client_socket, &ev);  
                }  
            }  
            else if(events[i].events & EPOLLOUT)//������д�¼�����������  
            {  
                char sendbuff[1024];  
                sprintf(sendbuff, "Hello, client fd: %d\n", client_socket);  
                int sendsize = send(client_socket, sendbuff, strlen(sendbuff)+1, MSG_NOSIGNAL);  
                if(sendsize <= 0)  
                {  
                    struct epoll_event event_del;  
                    event_del.data.fd = events[i].data.fd;  
                    event_del.events = 0;  
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, event_del.data.fd, &event_del);  
                }  
                else  
                {  
                    printf("Server reply msg ok! buffer: %s\n", sendbuff);  
                    struct epoll_event    ev;  
                    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;  
                    ev.data.fd = client_socket;     //��¼socket���  
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, client_socket, &ev);  
                }  
            }  
            else  
            {  
                cout << "EPOLL ERROR\n" <<endl;  
                epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);  
            }  
        }  
    }  
    
}  
  




bool InitServer(const char* pIp, int iPort,EpollServer epoll_server)  
{  
    epoll_server.m_iEpollFd = epoll_create(_MAX_SOCKFD_COUNT);  
  
    //���÷�����ģʽ  
    int opts = O_NONBLOCK;  
    if(fcntl(epoll_server.m_iEpollFd,F_SETFL,opts)<0)  
    {  
        printf("���÷�����ģʽʧ��!\n");  
        return false;  
    }  
  
    epoll_server.m_isock = socket(AF_INET,SOCK_STREAM,0);  
    if ( 0 > epoll_server.m_isock )  
    {  
        printf("socket error!\n");  
        return false;  
����}  
����  
����    sockaddr_in listen_addr;  
����    listen_addr.sin_family=AF_INET;  
����    listen_addr.sin_port=htons ( iPort );  
����    listen_addr.sin_addr.s_addr=htonl(INADDR_ANY);  
����    listen_addr.sin_addr.s_addr=inet_addr(pIp);  
����  
����    int ireuseadd_on = 1;//֧�ֶ˿ڸ���  
����    setsockopt(epoll_server.m_isock, SOL_SOCKET, SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on) );  
����  
����    if ( bind ( epoll_server.m_isock, ( sockaddr * ) &listen_addr,sizeof ( listen_addr ) ) !=0 )  
����    {  
����        printf("bind error\n");  
����        return false;  
����    }  
����  
����    if ( listen ( m_isock, 20) <0 )  
����    {  
����        printf("listen error!\n");  
����        return false;  
����    }  
����    else  
����    {  
����        printf("����˼�����...\n");  
����    }  
����  
����    // �����̣߳����̸߳�����տͻ������ӣ����뵽epoll��  
����    if ( pthread_create( &epoll_server.m_ListenThreadId, 0, ( void * ( * ) ( void * ) ) ListenThreadProcess, this ) != 0 )  
����    {  
����        printf("Server �����̴߳���ʧ��!!!");  
����        return false;  
����    }  
}  


int main()  
{  
        CEpollServer  theApp;  
        theApp.InitServer("127.0.0.1", 8000);  
        theApp.Run();  
  
        return 0;
}

/********************************************
 *��������  set_setsockopt_function(int server_sockfd ,int type)
 *���ܣ�    ����socket����:            SOL_SOCKET
 *���룺    int server_sockfd       ��Ҫ���õ�socket
 *          int type                   ��Ҫ���õ�socket���� 
               1:�������ñ��ض˿�
               2:������ʱ�����ӳٹر�Ϊ0�����̹ر�
               3.������ʱ�����ӳٹر�Ϊ10����ʱ�ر�
               4.�����Բ��������Ƿ���ڣ��׽��ֱ��
               5:���ճ�ʱ�����ͳ�ʱʱ���趨����������
               6:���պͷ��ͻ�������С�趨
               7:���պͷ��� ��������ˮƽλ���
 *ʾ��:  ������: 1��2
         �����ӣ�1,3,4,5           
               
 *�����    none
 *����ֵ��  -1       ����ʧ��
 *            0          ���óɹ�
 ********************************************/

int set_setsockopt_function(int server_sockfd ,int type)
{
    int ReUseFlag=1;
    int len=sizeof(int);

    struct linger short_linger,long_linger;      //���������ã����߳���������                                                               
    short_linger.l_onoff = 0;         //��ʱ״̬���رգ�.C����closesocket()��ǿ�ƹر�                                  
    short_linger.l_linger =0  ;       //���õȴ�ʱ��Ϊ0��,�����ӡ� ��ز�����ӣ�
    
    long_linger.l_onoff = 10;         //��ʱ״̬���򿪣�.C����closesocket()����ʱ�ر�                                  
    long_linger.l_linger =0  ;        //���õȴ�ʱ��Ϊ0��,�����ӡ� ��ز�����ӣ�


    int keepAlive = 1;      // ����keepalive����

    int keepIdle = 60;      // ��ʼ�״�KeepAlive̽��ǰ��TCP�ձ�ʱ��;���������60����û���κ���������,�����̽�� 
    int keepInterval = 5;   // ̽��ʱ������ʱ����Ϊ5 ��
    int keepCount = 3;      // ̽�Ⳣ�ԵĴ���.�����1��̽������յ���Ӧ��,���2�εĲ��ٷ�.

    int sndsize=8192;//��ʵ�Ļ�������СΪ�û����������

    struct timeval rcvtime,sndtime;
    rcvtime.tv_sec=10;//����Ϊ10��
    rcvtime.tv_usec=0;
    sndtime.tv_sec=10;//����Ϊ10��
    sndtime.tv_usec=0;


    int rcvlowat=10;//���ջ���������Ϳ����ֽ���
    int sndlowat=10;//���ͻ���������Ϳ����ֽ�����������  

    switch(type)
    {
      case 1:           //����˳����Ӷ����Ӷ�Ҫʹ���׽���ѡ��֪ͨ�ںˣ�����˿�æ����TCP״̬λ�� TIME_WAIT ���������ö˿ڶ����õȴ�60s��
          iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&ReUseFlag,sizeof(int));	//SO_REUSEADDRΪҪ���õ�ѡ��������closesocket()һ�㲻�������ر�socket��������TIME_WAIT�Ĺ��̡�BOOL bReuseaddr = TRUE;
          if(iRet<0)
          {
             perror("[ERROR]:setsockopt��SO_REUSEADDR");
             close(server_sockfd);
             return -1;
          }
          return 0;
      
      case 2:           // �������ж�ʱ����Ҫ�ӳٹر�(linger)�Ա�֤�������ݶ������䣬ֱ�ӹر�����socket��
          iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_LINGER,(char *) &short_linger, sizeof(struct linger));
          if (iRet < 0)
          {
             perror("[ERROR]:setsockopt��SO_LINGER");
             close(server_sockfd);
             return -1;//�������δ�����꣬ȴ������ closesocket()����ʱ����ǿ�ƹرգ����ٷ��ͣ��������ӣ���
          }
          return 0;
      case 3:         // �������ж�ʱ����Ҫ�ӳٹر�(linger)�Ա�֤�������ݶ������䣬������Ҫ��SO_LINGER���ѡ��(������)//�������δ�����꣬ȴ������ closesocket()����ʱ������ʱ>�رգ�������ɺ󣻣������ӣ���                                                                                                                                                           iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_LINGER,(char *) &long_linger, sizeof(struct linger));
          iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_LINGER,(char *) &long_linger, sizeof(struct linger));
          if (iRet < 0)
          {
             perror("[ERROR]:setsockopt��SO_LINGER");
             close(server_sockfd);
             return -1;
          }
          return 0;
      
      
      case 4:           //����������ʹ�ã��ײ��׽��ֱ��ȱ���ⲻ��ʱ���ᱻ����ǽ���ص���������Ʋ����á�
          iRet =setsockopt(server_sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(int));//����̽��2��Сʱ̽��һ�ζԷ������Ƿ����
          if (iRet<0)
          {
             fprintf(stderr,"����keepaliveʧ��");
             close(server_sockfd); 
             return -1;
          }
          ////��������ʱ��------��SOL_TCP�������״�̽��ʱ�䣬̽��ʱ������̽��ʱ�����
          //   iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(int));//�״�̽��ǰʱ�䣻
          //   if (iRet<0)
          //   {
          //      fprintf(stderr,"���� ��ʼ�״�KeepAlive̽��ǰ��TCP�ձ�ʱ�� ʧ��");
          //      close(server_sockfd); 
          //      return -1;
          //   }
          //   iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(int));//�ٴ�̽��ʱ����
          //   if (iRet<0)
          //   {
          //      fprintf(stderr,"���� ����KeepAlive̽����ʱ���� ʧ��");
          //      close(server_sockfd); 
          //      return -1;
          //   }
          //   iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(int)); //̽�����
          //   if (iRet<0)
          //   {
          //      fprintf(stderr,"���� �ж��Ͽ�ǰ��KeepAlive̽����� ʧ��");
          //      close(server_sockfd); 
          //      return -1;
          //   }  
          return 0;
      
      case 5:           //socket�еĳ�ʱ���⣺���ͳ�ʱʱ������ճ�ʱʱ��SO_RCVTIMEO,SO_SNDTIMEOĬ�������recv����û�н��յ����ݻ�һֱ������
          iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_SNDTIMEO,&sndtime,sizeof(struct timeval));//���ͳ�ʱʱ���趨
          if (iRet < 0)
          {
             perror("SO_SNDTIMEO error");
             close(server_sockfd); 
             return -1;
          }
          iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_RCVTIMEO,&rcvtime,sizeof(struct timeval));//���ճ�ʱʱ���趨
          if (iRet < 0)
          {
             perror("SO_RCVTIMEO");
             close(server_sockfd); 
             return -1;
          }
          return 0;
      
      case 6:               //���ý��ܻ�������С  
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
          
          // if (setsockopt(server_sockfd, SOL_SOCKET, SO_SNDLOWAT, &rcvbuf_l, sizeof(rcvbuf))<0) //���͵ͳ��޶ȣ�Ĭ��Ϊ1�ֽ�
          // {
          // perror("setsockopt: SO_SNDLOWAT");
          // close(server_sockfd);
          // return -1;
          // }
          return 0;
      
      case 7:  //���ջ�������ˮƽλ���
          iRet =setsockopt(server_sockfd,SOL_SOCKET,SO_RCVLOWAT,&rcvlowat,sizeof(rcvlowat));
          if (iRet<0)
          {
             perror("SO_RCVLOWAT ERROR");
             close(server_sockfd); 
             return -1;
          }
          iRet =setsockopt(server_sockfd,SOL_SOCKET,SO_SNDLOWAT,&sndlowat,sizeof(sndlowat));
          if (iRet<0)
          {
             perror("SO_SNDLOWAT ERROR");
             close(server_sockfd); 
             return -1;
          }
      default:
          perror("[ERROR]:setsockopt type not define! ");
          close(server_sockfd); 
          return -1;
          
    }
    return 0;
}


/********************************************
 *��������  socket_listen_bind-----��Է�������
 *���ܣ�    ����һ��socket������socket���ԣ���IP��ַ���˿ڣ���������
 *���룺    int Port       ��Ҫ�󶨼����ı��ض˿ں�
            int type       ���ó�����1 ������2.
            blockType      Ĭ������0��1��������
 *�����    none
 *����ֵ��  ���ش����ɹ���socketֵ
 *ע�⣺    ����socket���ԡ�ͨ��int set_setsockopt_function(int server_sockfd ,int type)����type�����趨
 ********************************************/

int socket_listen_bind(int Port,int type,int blockType)//����һ��socket���ú���socket()������socket����(��ѡ�ú���setsockopt())��; ��IP��ַ���˿ڵ���Ϣ��socket�ϣ��ú���bind();�����������ú���listen()��
{
    int server_sockfd=0;
    int iRet=0;
    struct   sockaddr_in server_address;
    
    server_sockfd=socket(AF_INET,SOCK_STREAM,0);                   //����һ��socket���ú���socket()����ipv4 ,�����Զ�ƥ�䣩
    if(server_sockfd<0)
    {
        perror("SOCKET ERROR");
        return -1;
    }
    iRet=set_setsockopt_function(server_sockfd ,1);
    if(iRet<0)
    {
        perror("[ERROR]:setsockopt-et_setsockopt_function");
        close(server_sockfd);
        return -1;
    }
    
    if(type==2)//������
    {
        iRet=set_setsockopt_function(server_sockfd ,2);
        if(iRet<0)
        {
            perror("[ERROR]:set_setsockopt_function!");
            return -1;
        }
        //iRet=set_setsockopt_function(server_sockfd ,5);//���ûᵼ�������ж�
    } 
    else if((type==1) //������
    {
        iRet=set_setsockopt_function(server_sockfd ,3);
        if(iRet<0)
        {
            perror("[ERROR]:set_setsockopt_function!");
            return -1;
        }
        iRet=set_setsockopt_function(server_sockfd ,4);//���ʹ��������    
        if(iRet<0)
        {
            perror("[ERROR]:set_setsockopt_function!");
            return -1;
        }
    }
    if(blockType==1)//������
    {
       setNonBlocking(server_sockfd);//����Ϊ������
    }
    
    bzero(&server_address,sizeof(struct sockaddr_in));
    server_address.sin_family=AF_INET;
    server_address.sin_addr.s_addr=htonl(INADDR_ANY);         //������������
    server_address.sin_port=htons(Port);                      //�˿ڴ��ⲿ���Ӻ�����ڴ�����
    iRet=bind(server_sockfd,(struct sockaddr *)&server_address,sizeof(struct sockaddr_in));
    if(iRet<0)
    {
        fprintf(stdout,"Bind port[%d] error:%s\n\a",Port,strerror(errno));
        perror("bind error");
        close(server_sockfd);
        return -1;
    }
    else
      printf("IP ��ַ�Ͷ˿ڰ󶨳ɹ�\n");
    if(listen(server_sockfd,5)<0)
    {
        perror("listen error");
        close(server_sockfd);
        return -1;
    }
    return(server_sockfd);
}




int main(int argc, char **argv)
{
     EpollServer.server_sockfd=socket_listen_bind(LISTEN_PORT,2,0);//������2,����0
     if(server_sockfd<=0)
     {
        printf("ERROR:listen port is:[%d] in use,can not listen\n",LISTEN_PORT);
        exit(0);
     }
     else
       printf("IP ��ַ�Ͷ˿ڰ󶨳ɹ�,��������ɹ���\n");

     EpollServer.epoll_fd = epoll_create(MAXEPOLLSIZE);//���� epoll ������Ѽ��� socket ���뵽 epoll ������
     setNonBlocking(EpollServer.epoll_fd);             //����Ϊ������
���� // �����̣߳����̸߳�����տͻ������ӣ����뵽epoll��  
���� if ( pthread_create( &EpollServer.listenThreadId, 0, ( void * ( * ) ( void * ) ) ListenThreadProcess, &EpollServer ) != 0 )  
���� {  
����     printf("Server �����̴߳���ʧ��!!!");  
����     return false;  
���� } 






    len = sizeof(struct sockaddr_in);
    
    epEvent.events = EPOLLIN | EPOLLET;//�пɶ����ݣ����ش���
    epEvent.data.fd = server_sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sockfd, &ev) < 0)//ע��epoll�¼�
    {
        fprintf(stderr, "epoll set insertion error: fd=%d\n", listener);
        return -1;
    }
    else
    {
       printf("���� socket ���� epoll �ɹ���\n");
    }
  curfds = 1;
  while (1)
  {
     //�ȴ����¼�����
     nfds = epoll_wait(epoll_fd, events, curfds, -1); //ÿ����һ��socket��curfds��һ��֪ͨ�ں����events(�����Ա�ĸ���)  ��-1��ʾ���û����Ϣ֪ͨ������
     if (nfds == -1)
     {
        perror("epoll_wait");
        break;
     }
     else if(nfds==0)
      printf("epoll_wait��ѯI/O�¼��ķ���,̽�ⳬʱ.��Ҫ�����¼�����ĿΪ[%d]\n",nfds);
     else
       //printf("��Ҫ�����¼�����ĿΪ[%d]\n",nfds);

     /* ���������¼� */
     for (n = 0; n < nfds; ++n)
     {
        if (events[n].data.fd == listener)  //listenerΪ����socket������������µ�����
        {
           new_fd = accept(listener, (struct sockaddr *) &their_addr,&len);
           if (new_fd < 0)
           {
              perror("accept");
              continue;
           }
           else
           {
               printf("new connection[%d] from [%s],port [%d]\n",new_fd,inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port));
           }

           setNonBlocking(new_fd); //����Ϊ��������������
           ev.events = EPOLLIN | EPOLLET;   //epoll_IN,epoll_ET-server   ���ش���
           ev.data.fd = new_fd;
           if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev) < 0)
           {
              fprintf(stderr, "������socket[%d]����epoll�ɹ�ʧ��![%s]\n",new_fd, strerror(errno));
              return -1;
           }
           curfds++;
           printf("������socket[%d]����epoll�ɹ�[%d]\n",new_fd,curfds);
        }
        else
        {
           //printf("����Ϣ��������Ϣ������[%d]\n",events[n].data.fd);
           ret = handle_message(events[n].data.fd);
           if (ret < 1 && errno != 11)
           {
              epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd,&ev);
              curfds--;
              close(events[n].data.fd);
              printf("���ӹر�socket[%d]���ر�epoll�ɹ�[%d]\n",events[n].data.fd,curfds);

           }
        }

     }
  }
  close(listener);
  return 0;
}
