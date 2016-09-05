/************关于本文档********************************************
 * *filename: epoll-ET-server.c
 * *purpose: 演示epoll处理海量socket连接的方法
 * *Linux爱好者 Linux知识传播者 SOHO族 开发者 最擅长C语言
 * *date time:2007-01-31 21:00
 * *Note: 任何人可以任意复制代码并运用这些文档，当然包括你的商业用途
 * *但请遵循GPL
 * *Thanks to:Google
 * *Hope:希望越来越多的人贡献自己的力量，为科学技术发展出力
 * *科技站在巨人的肩膀上进步更快！感谢有开源前辈的贡献！
 * *********************************************************************/

#include "epoll-ET-server.h"



/*
 * setNonBlocking - 设置句柄为非阻塞方式
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
 * handle_message - 处理每个 socket 上的消息收发
 * */
int handle_message(int new_fd)
{
  char buf[1024],sumbuf[MAXBUF];
  int buflen,sumlen=0,readflag=1;

  bzero(buf, sizeof(buf));
  bzero(sumbuf, sizeof(sumbuf));

  while(readflag)           //ET模型特别注意读写
  {
    buflen = recv(new_fd, buf, sizeof(buf), 0);
    if(buflen < 0)
    {
      //由于是非阻塞的模式,所以当errno为EAGAIN时,表示当前缓冲区已无数据可读
      // 在这里就当作是该次事件已处理处.
      if(errno== EAGAIN || errno == EINTR || errno == EWOULDBLOCK)  //即当buflen<0且errno=EAGAIN时，表示没有数据了。(读/写都是这样)
      {
        printf("没有数据读写了\n");
        break;
      }
      else
      {
        printf("消息接收失败！错误代码是%d，错误信息是'%s'\n",errno, strerror(errno));
        close(new_fd);
        return -1;                 //真的失败了。
      }
    }
    else if(buflen == 0)//这里表示对端的socket已正常关闭.
    {
      printf("对端的socket已正常关闭\n");
      return 0;
    }
    else
    {
      //printf("%d接收消息成功:'%s'，共%d个字节的数据\n",new_fd, buf, buflen);
      memcpy(sumbuf+sumlen,buf,buflen);
      sumlen+=buflen;
      bzero(buf, sizeof(buf));
    }

    if(buflen == sizeof(buf))
      readflag = 1;   //需要再次读取(有可能是因为数据缓冲区buf太小，所以数据没有读完)
    else
      readflag =0;    //不需要再次读取(当buflen
  }
  printf("---------socket[%d]接收消息成功LEN[%d]-[%s]\n",new_fd, sumlen, sumbuf);
   /* 处理每个新连接上的数据收发结束 */
   return sumlen;
}

// 监听线程  
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
            printf("Server Accept失败!, client_socket: %d\n", client_socket);  
            continue;  
        }  
        else
        {  
            struct epoll_event    ev;  
            ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;  
            ev.data.fd = new_socket;     //记录socket句柄  
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
            char buffer[1024];//每次收发的字节数小于1024字节  
            memset(buffer, 0, 1024);  
            if (events[i].events & EPOLLIN)//监听到读事件，接收数据  
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
                    ev.data.fd = client_socket;     //记录socket句柄  
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, client_socket, &ev);  
                }  
            }  
            else if(events[i].events & EPOLLOUT)//监听到写事件，发送数据  
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
                    ev.data.fd = client_socket;     //记录socket句柄  
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
  
    //设置非阻塞模式  
    int opts = O_NONBLOCK;  
    if(fcntl(epoll_server.m_iEpollFd,F_SETFL,opts)<0)  
    {  
        printf("设置非阻塞模式失败!\n");  
        return false;  
    }  
  
    epoll_server.m_isock = socket(AF_INET,SOCK_STREAM,0);  
    if ( 0 > epoll_server.m_isock )  
    {  
        printf("socket error!\n");  
        return false;  
　　}  
　　  
　　    sockaddr_in listen_addr;  
　　    listen_addr.sin_family=AF_INET;  
　　    listen_addr.sin_port=htons ( iPort );  
　　    listen_addr.sin_addr.s_addr=htonl(INADDR_ANY);  
　　    listen_addr.sin_addr.s_addr=inet_addr(pIp);  
　　  
　　    int ireuseadd_on = 1;//支持端口复用  
　　    setsockopt(epoll_server.m_isock, SOL_SOCKET, SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on) );  
　　  
　　    if ( bind ( epoll_server.m_isock, ( sockaddr * ) &listen_addr,sizeof ( listen_addr ) ) !=0 )  
　　    {  
　　        printf("bind error\n");  
　　        return false;  
　　    }  
　　  
　　    if ( listen ( m_isock, 20) <0 )  
　　    {  
　　        printf("listen error!\n");  
　　        return false;  
　　    }  
　　    else  
　　    {  
　　        printf("服务端监听中...\n");  
　　    }  
　　  
　　    // 监听线程，此线程负责接收客户端连接，加入到epoll中  
　　    if ( pthread_create( &epoll_server.m_ListenThreadId, 0, ( void * ( * ) ( void * ) ) ListenThreadProcess, this ) != 0 )  
　　    {  
　　        printf("Server 监听线程创建失败!!!");  
　　        return false;  
　　    }  
}  


int main()  
{  
        CEpollServer  theApp;  
        theApp.InitServer("127.0.0.1", 8000);  
        theApp.Run();  
  
        return 0;
}

/********************************************
 *函数名：  set_setsockopt_function(int server_sockfd ,int type)
 *功能：    设置socket属性:            SOL_SOCKET
 *输入：    int server_sockfd       需要设置的socket
 *          int type                   需要设置的socket类型 
               1:允许重用本地端口
               2:短连接时设置延迟关闭为0，立刻关闭
               3.长连接时设置延迟关闭为10，延时关闭
               4.周期性测试连接是否存在，套接字保活。
               5:接收超时，发送超时时间设定。避免阻塞
               6:接收和发送缓冲区大小设定
               7:接收和发送 缓冲区低水平位标记
 *示例:  短连接: 1，2
         长连接：1,3,4,5           
               
 *输出：    none
 *返回值：  -1       设置失败
 *            0          设置成功
 ********************************************/

int set_setsockopt_function(int server_sockfd ,int type)
{
    int ReUseFlag=1;
    int len=sizeof(int);

    struct linger short_linger,long_linger;      //短连接设置，或者长连接设置                                                               
    short_linger.l_onoff = 0;         //延时状态（关闭）.C调用closesocket()后强制关闭                                  
    short_linger.l_linger =0  ;       //设置等待时间为0秒,短连接。 （夭折连接）
    
    long_linger.l_onoff = 10;         //延时状态（打开）.C调用closesocket()后延时关闭                                  
    long_linger.l_linger =0  ;        //设置等待时间为0秒,短连接。 （夭折连接）


    int keepAlive = 1;      // 开启keepalive属性

    int keepIdle = 60;      // 开始首次KeepAlive探测前的TCP空闭时间;如该连接在60秒内没有任何数据往来,则进行探测 
    int keepInterval = 5;   // 探测时发包的时间间隔为5 秒
    int keepCount = 3;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.

    int sndsize=8192;//真实的缓冲区大小为用户输入的两倍

    struct timeval rcvtime,sndtime;
    rcvtime.tv_sec=10;//设置为10秒
    rcvtime.tv_usec=0;
    sndtime.tv_sec=10;//设置为10秒
    sndtime.tv_usec=0;


    int rcvlowat=10;//接收缓冲区的最低空闲字节数
    int sndlowat=10;//发送缓冲区的最低空闲字节数不能设置  

    switch(type)
    {
      case 1:           //服务端长连接短连接都要使用套接字选项通知内核，如果端口忙，但TCP状态位于 TIME_WAIT ，可以重用端口而不用等待60s。
          iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&ReUseFlag,sizeof(int));	//SO_REUSEADDR为要设置的选项名调用closesocket()一般不会立即关闭socket，而经历TIME_WAIT的过程。BOOL bReuseaddr = TRUE;
          if(iRet<0)
          {
             perror("[ERROR]:setsockopt：SO_REUSEADDR");
             close(server_sockfd);
             return -1;
          }
          return 0;
      
      case 2:           // 当连接中断时，需要延迟关闭(linger)以保证所有数据都被传输，直接关闭数据socket。
          iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_LINGER,(char *) &short_linger, sizeof(struct linger));
          if (iRet < 0)
          {
             perror("[ERROR]:setsockopt：SO_LINGER");
             close(server_sockfd);
             return -1;//针对数据未发送完，却调用了 closesocket()；此时做法强制关闭，不再发送；（短连接）。
          }
          return 0;
      case 3:         // 当连接中断时，需要延迟关闭(linger)以保证所有数据都被传输，所以需要打开SO_LINGER这个选项(长连接)//针对数据未发送完，却调用了 closesocket()；此时做法延时>关闭，发送完成后；（长连接）。                                                                                                                                                           iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_LINGER,(char *) &long_linger, sizeof(struct linger));
          iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_LINGER,(char *) &long_linger, sizeof(struct linger));
          if (iRet < 0)
          {
             perror("[ERROR]:setsockopt：SO_LINGER");
             close(server_sockfd);
             return -1;
          }
          return 0;
      
      
      case 4:           //长连接设置使用：底层套接字保活。缺点检测不及时，会被防火墙拦截掉，保活机制不好用。
          iRet =setsockopt(server_sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(int));//开启探测2个小时探测一次对方主机是否存在
          if (iRet<0)
          {
             fprintf(stderr,"设置keepalive失败");
             close(server_sockfd); 
             return -1;
          }
          ////重新设置时间------用SOL_TCP来设置首次探测时间，探测时间间隔，探测时间次数
          //   iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(int));//首次探测前时间；
          //   if (iRet<0)
          //   {
          //      fprintf(stderr,"设置 开始首次KeepAlive探测前的TCP空闭时间 失败");
          //      close(server_sockfd); 
          //      return -1;
          //   }
          //   iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(int));//再次探测时间间隔
          //   if (iRet<0)
          //   {
          //      fprintf(stderr,"设置 两次KeepAlive探测间的时间间隔 失败");
          //      close(server_sockfd); 
          //      return -1;
          //   }
          //   iRet =setsockopt(server_sockfd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(int)); //探测次数
          //   if (iRet<0)
          //   {
          //      fprintf(stderr,"设置 判定断开前的KeepAlive探测次数 失败");
          //      close(server_sockfd); 
          //      return -1;
          //   }  
          return 0;
      
      case 5:           //socket中的超时问题：发送超时时延与接收超时时延SO_RCVTIMEO,SO_SNDTIMEO默认情况下recv函数没有接收到数据会一直阻塞的
          iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_SNDTIMEO,&sndtime,sizeof(struct timeval));//发送超时时间设定
          if (iRet < 0)
          {
             perror("SO_SNDTIMEO error");
             close(server_sockfd); 
             return -1;
          }
          iRet=setsockopt(server_sockfd,SOL_SOCKET,SO_RCVTIMEO,&rcvtime,sizeof(struct timeval));//接收超时时间设定
          if (iRet < 0)
          {
             perror("SO_RCVTIMEO");
             close(server_sockfd); 
             return -1;
          }
          return 0;
      
      case 6:               //设置接受缓冲区大小  
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
          
          // if (setsockopt(server_sockfd, SOL_SOCKET, SO_SNDLOWAT, &rcvbuf_l, sizeof(rcvbuf))<0) //发送低潮限度：默认为1字节
          // {
          // perror("setsockopt: SO_SNDLOWAT");
          // close(server_sockfd);
          // return -1;
          // }
          return 0;
      
      case 7:  //接收缓冲区低水平位标记
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
 *函数名：  socket_listen_bind-----针对服务器端
 *功能：    创建一个socket，设置socket属性；绑定IP地址、端口；开启监听
 *输入：    int Port       需要绑定监听的本地端口号
            int type       设置长连接1 短连接2.
            blockType      默认阻塞0，1非阻塞。
 *输出：    none
 *返回值：  返回创建成功的socket值
 *注意：    设置socket属性。通过int set_setsockopt_function(int server_sockfd ,int type)函数type进行设定
 ********************************************/

int socket_listen_bind(int Port,int type,int blockType)//创建一个socket，用函数socket()；设置socket属性(可选用函数setsockopt())，; 绑定IP地址、端口等信息到socket上，用函数bind();开启监听，用函数listen()；
{
    int server_sockfd=0;
    int iRet=0;
    struct   sockaddr_in server_address;
    
    server_sockfd=socket(AF_INET,SOCK_STREAM,0);                   //创建一个socket，用函数socket()；（ipv4 ,流，自动匹配）
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
    
    if(type==2)//短连接
    {
        iRet=set_setsockopt_function(server_sockfd ,2);
        if(iRet<0)
        {
            perror("[ERROR]:set_setsockopt_function!");
            return -1;
        }
        //iRet=set_setsockopt_function(server_sockfd ,5);//设置会导致连接中断
    } 
    else if((type==1) //长连接
    {
        iRet=set_setsockopt_function(server_sockfd ,3);
        if(iRet<0)
        {
            perror("[ERROR]:set_setsockopt_function!");
            return -1;
        }
        iRet=set_setsockopt_function(server_sockfd ,4);//最好使用心跳包    
        if(iRet<0)
        {
            perror("[ERROR]:set_setsockopt_function!");
            return -1;
        }
    }
    if(blockType==1)//非阻塞
    {
       setNonBlocking(server_sockfd);//设置为非阻塞
    }
    
    bzero(&server_address,sizeof(struct sockaddr_in));
    server_address.sin_family=AF_INET;
    server_address.sin_addr.s_addr=htonl(INADDR_ANY);         //本机做服务器
    server_address.sin_port=htons(Port);                      //端口从外部，从函数入口处传入
    iRet=bind(server_sockfd,(struct sockaddr *)&server_address,sizeof(struct sockaddr_in));
    if(iRet<0)
    {
        fprintf(stdout,"Bind port[%d] error:%s\n\a",Port,strerror(errno));
        perror("bind error");
        close(server_sockfd);
        return -1;
    }
    else
      printf("IP 地址和端口绑定成功\n");
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
     EpollServer.server_sockfd=socket_listen_bind(LISTEN_PORT,2,0);//短连接2,阻塞0
     if(server_sockfd<=0)
     {
        printf("ERROR:listen port is:[%d] in use,can not listen\n",LISTEN_PORT);
        exit(0);
     }
     else
       printf("IP 地址和端口绑定成功,开启服务成功！\n");

     EpollServer.epoll_fd = epoll_create(MAXEPOLLSIZE);//创建 epoll 句柄，把监听 socket 加入到 epoll 集合里
     setNonBlocking(EpollServer.epoll_fd);             //设置为非阻塞
　　 // 监听线程，此线程负责接收客户端连接，加入到epoll中  
　　 if ( pthread_create( &EpollServer.listenThreadId, 0, ( void * ( * ) ( void * ) ) ListenThreadProcess, &EpollServer ) != 0 )  
　　 {  
　　     printf("Server 监听线程创建失败!!!");  
　　     return false;  
　　 } 






    len = sizeof(struct sockaddr_in);
    
    epEvent.events = EPOLLIN | EPOLLET;//有可读数据，边沿触发
    epEvent.data.fd = server_sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sockfd, &ev) < 0)//注册epoll事件
    {
        fprintf(stderr, "epoll set insertion error: fd=%d\n", listener);
        return -1;
    }
    else
    {
       printf("监听 socket 加入 epoll 成功！\n");
    }
  curfds = 1;
  while (1)
  {
     //等待有事件发生
     nfds = epoll_wait(epoll_fd, events, curfds, -1); //每次来一个socket，curfds加一。通知内核这个events(数组成员的个数)  ；-1表示如果没有消息通知则阻塞
     if (nfds == -1)
     {
        perror("epoll_wait");
        break;
     }
     else if(nfds==0)
      printf("epoll_wait轮询I/O事件的发生,探测超时.需要处理事件的数目为[%d]\n",nfds);
     else
       //printf("需要处理事件的数目为[%d]\n",nfds);

     /* 处理所有事件 */
     for (n = 0; n < nfds; ++n)
     {
        if (events[n].data.fd == listener)  //listener为监听socket。若相等则有新的连接
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

           setNonBlocking(new_fd); //设置为非阻塞，不休眠
           ev.events = EPOLLIN | EPOLLET;   //epoll_IN,epoll_ET-server   边沿触发
           ev.data.fd = new_fd;
           if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev) < 0)
           {
              fprintf(stderr, "新连接socket[%d]加入epoll成功失败![%s]\n",new_fd, strerror(errno));
              return -1;
           }
           curfds++;
           printf("新连接socket[%d]加入epoll成功[%d]\n",new_fd,curfds);
        }
        else
        {
           //printf("新消息来到，消息处理中[%d]\n",events[n].data.fd);
           ret = handle_message(events[n].data.fd);
           if (ret < 1 && errno != 11)
           {
              epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd,&ev);
              curfds--;
              close(events[n].data.fd);
              printf("连接关闭socket[%d]，关闭epoll成功[%d]\n",events[n].data.fd,curfds);

           }
        }

     }
  }
  close(listener);
  return 0;
}
