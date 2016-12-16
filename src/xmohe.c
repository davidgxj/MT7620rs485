#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <termios.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <pthread.h>
#include<stdbool.h>
#include<signal.h>
#include<sys/wait.h>
#include "serial.h"
#include "getcfg.h"
#include "list.h"


char cliend_buf[128];//接收数据缓存放置SOCKET数据
int sockread_len;//客户端读取的字节数

char ipaddr[16];//用于存放获取的Ip地址
char dns_addr[50];//用于存放存放远程域名地址
int ser_port;//远程端口
int client_port;//本地端口
char RS232_BUF;//串口端口
int RS232_BAUD;//串口波特率

int serial_fd;//串口FD


/*struct Infodata {
	int socfd;
	char * string;
	}INFODATA;
*/
/*function prototype*/

void setnonblocking(int sock)
{
	int opts;
	opts=fcntl(sock,F_GETFL);
	if(opts<0)
	{
		perror("fcntl GETFL");
		exit(1);
	}
	opts=opts|O_NONBLOCK;
	if(fcntl(sock,F_SETFL,opts)<0)
	{
		perror("fcntl SETFL");
		exit(1);
	}
}

void* ClientThread(void *recv_id)
{
	int rev_fd = *(int *)recv_id;
	int j;	
	/* 接收客户端发来的数据并显示出来 */
		
		if (sockread_len <= 0)
		{
			close(rev_fd);
			pthread_exit(NULL);
		}
		else
		{
			//写入选中的串口中
			//write(*recv_id,buf,ret);
			if(serial_fd>2)	
				{
					write(serial_fd,cliend_buf,sockread_len);
					printf("rcv socket:%s\n",cliend_buf);
				}
			}
}	


int main()
{
  pthread_t  Client_ID;
  //创建线程
  signal(SIGCHLD,handle_sigchld);
  createNodelist();  //创建链表用于存储FD
  //串口初始化获取路径值设置串口以及远程连接端口
  GetConfigValue("/etc/config/rs232" );
  
  printf("client ipport is %d\n",client_port);//本地端口
  printf("server ipport is %d\n",ser_port);//远程端口
  printf("the server ip is %s\n",ipaddr);

  
   /*声明epoll_event结构体的变量，ev用于注册事件，events
   数组用于回传要处理的事件*/
   
   struct epoll_event ev,events[20];//数组是用于回传接收的事件
   /*生成用于处理accept的epoll专用
   的文件描述符，指定生成描述符的最大范围为256*/
  int epoll_instance=epoll_create(200);//size is unused nowadays

  int  listenfd = socket(AF_INET,SOCK_STREAM,0);
   if(listenfd <0)
   {
      perror("error opening socket");
      return -1;
   }
   //setnonblocking(listenfd);//把用于监听的socket设置成非阻塞方式
   ev.data.fd=listenfd;//设置与要处理的事件相关的文件描述符
   ev.events=EPOLLIN|EPOLLET;//设置要处理的事件类型
   epoll_ctl(epoll_instance,EPOLL_CTL_ADD,listenfd,&ev);//注册epoll事件
/*
本地服务器设置，绑定端口，如果是客户端直接读取远程的ip
*/
   struct sockaddr_in client_addr;
   memset(&client_addr,0,sizeof(client_addr));
   client_addr.sin_family = AF_INET;
   client_addr.sin_addr.s_addr = INADDR_ANY;
   client_addr.sin_port = htons((uint16_t )client_port);//本地端口
/*解决端口复用问题*/
int opt = 1;
int sizeInt_len = sizeof(opt);
  if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,(char *)&opt,sizeInt_len)==-1)
  	{       
  	perror("setsockopt");       
	return -1;    
	}
  int ret = bind(listenfd,(struct sockaddr*)&client_addr,sizeof(client_addr));
   if(ret <0)
   {
      perror("Error on binding");
      return -1;
   } 
   ret = listen(listenfd,5);//backlog
   if(ret !=0)
   {
       perror("Error on listening");
       return -1;
   }

/*连接远程主机
*/
struct timeval  timeout = {3,0};
socklen_t len_time = sizeof(timeout);

//创建远程SOCKET
  int  ser_lister = socket(AF_INET,SOCK_STREAM,0);
  setsockopt(ser_lister, SOL_SOCKET, SO_SNDTIMEO, &timeout,len_time);
  if(ser_lister<0)
  	{
  		perror("error opening server_socket\n");
     		return -1;
  	}
  //远程服务器
   struct sockaddr_in  ser_addr;
   memset(&ser_addr,0,sizeof(ser_addr));
   ser_addr.sin_family = AF_INET;
   //Set ser_ip
   	if( inet_aton(ipaddr, &ser_addr.sin_addr)==0)
  	{
  	perror("connect server erro!\n");
	//return -1;
	 }
	else
	{
  		//设置端口
  		ser_addr.sin_port = htons((uint16_t )ser_port);
   		/*设置端口复用*/
   		int opt_1= 1;
  		int sizeInt_len_1 = sizeof(opt);
  		if(setsockopt(ser_lister,SOL_SOCKET,SO_REUSEADDR,(char *)&opt_1,sizeInt_len_1)==-1)
  			{       
  				perror("setsockopt\n");      
				return -1;    
			}
   		if(connect(ser_lister,(struct sockaddr*)&ser_addr,sizeof(ser_addr))<0)
    			{
    				perror("connect serverip erro!\n");       
				//return -1;   
    			}
		  /*将远程主机加入epoll 事件集合*/
   		ev.data.fd=ser_lister ;//设置与要处理的事件相关的文件描述符
   		ev.events=EPOLLIN|EPOLLET;//设置要处理的事件类型
   		epoll_ctl(epoll_instance,EPOLL_CTL_ADD,ser_lister ,&ev);//注册epoll事件
   		   /*将新建的SERVER节点加入链表中*/
  		Node * sernode=(Node*)malloc(sizeof(Node));
  		sernode->fd_data=ser_lister;
  		sernode->pNext=NULL;
 		addNode(sernode);
	}
//串口设备文件
	if('1'==RS232_BUF)
		{
		  if(( serial_fd = open_port(serial_fd, 0))<0)
 			{
 			perror("open_port error");
			return;
 			}
		int k;
   		if(( k=set_opt(serial_fd,RS232_BAUD,8,'N',1))<0)
			{
			perror("set_opt error\n");
			return;
			}
		if(serial_fd>2)
			{
			//将串口事件加入到epoll 中
			ev.data.fd=serial_fd;//设置与要处理的事件相关的文件描述符
   			ev.events=EPOLLIN|EPOLLET;//设置要处理的事件类型读，边沿触发
   			epoll_ctl(epoll_instance,EPOLL_CTL_ADD,serial_fd,&ev);//注册epoll事件
			printf("serial_fd is %d\n",serial_fd);	
			}
		}
   while(1)
   {
    int nfound=epoll_wait(epoll_instance,events,20,1000);//等待epoll事件的发生,返回事件数
   	  if(nfound==0)
   	  {
   	  	//printf(".");
   	  	fflush(stdout);
		continue;
   	  }
	int n_poll;
    for(n_poll=0;n_poll<nfound;n_poll++)
   	  {
   	  	if(events[n_poll].data.fd==listenfd)/*本地端口有新的链接*/
   	  	{
   	  		struct sockaddr_in cliaddr;//连接到本地的客户端
   	  		uint32_t  len = sizeof(cliaddr);
   	  		int connfd=accept(listenfd,(struct sockaddr*)&cliaddr,&len);
   	  		printf("connection from host %s,port %d,sockfd is %d\n",
             		inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port),connfd);
   	  		  //setnonblocking(connfd);
   	  		  
   	  		  ev.data.fd=connfd;//设置用于读操作的文件描述符
   	  		  ev.events=EPOLLIN|EPOLLET;//设置用于注册的读操作事件
   	  		  epoll_ctl(epoll_instance,EPOLL_CTL_ADD,connfd,&ev);//注册ev事件
			  printf("add %d to epoll\n",connfd);
			  
			//info_data.socfd=connfd;
			//printf("connect is %d\n,info_data.socfd");
			//添加系统信息
			//添加到链表
			Node * clientnode=(Node*)malloc(sizeof(Node));
			clientnode->fd_data=connfd;
			clientnode->pNext=NULL;
			addNode(clientnode);
   	  	}
   	  	else if(events[n_poll].events&EPOLLIN)/*读事件*/
   	  		{    
   	  			if(events[n_poll].data.fd==serial_fd)
   	  			{
   	  				int read_cont;//读取到串口的字节数
					char serial_buf[128];//用于存放串口的数据
   	  				if((read_cont=read(serial_fd,serial_buf,128))>0)
						{
						serialdata_handle(serial_buf,read_cont);
						printf("rev %d from serial is : %s\n",read_cont,serial_buf);
						memset(serial_buf,0,sizeof(serial_buf));
						}
   	  			}
				else if(events[n_poll].data.fd==ser_lister)
				{
					char revser_buf[128];
					memset(revser_buf,0,sizeof(revser_buf));
					int reser_len ;
					reser_len = read(ser_lister,revser_buf,sizeof(revser_buf)-1);
					if(reser_len<=0)
					{
   	  		  	  		printf("del server socket\n");
						printf("reconect the server again!\n");
						if(connect(ser_lister,(struct sockaddr*)&ser_addr,sizeof(ser_addr))<0)
    						{
    						   	ev.data.fd=ser_lister;//设置用于读操作的文件描述符
   	  		  				ev.events=EPOLLIN|EPOLLET;//设置用于注册的读操作事件
   	  		  				epoll_ctl(epoll_instance,EPOLL_CTL_DEL,ser_lister,&ev);//删除事件
    	 						perror("connect serverip erro!\n");
					 		if(deleteNode(ser_lister)>0)
				 			printf("delete the server from list\n");
							//return -1;   
    						}
					}
					else
					{
						printf("recv %d,from server is %s",reser_len,revser_buf);
						write(serial_fd,revser_buf,reser_len);
					}
				}
					//判断是否是SOCKET事件
				else{
					int sockfd=events[n_poll].data.fd;	
   	  				memset(cliend_buf,0,sizeof(cliend_buf));
					sockread_len=read(sockfd,cliend_buf,sizeof(cliend_buf)-1);
   	  				if(sockread_len<=0)
   	  		 		 {
   	  		   	  		ev.data.fd=sockfd;
   	  		  	  		ev.events=EPOLLIN|EPOLLET;
   	  		   	 		epoll_ctl(epoll_instance,EPOLL_CTL_DEL,sockfd,&ev);
						 if(deleteNode(sockfd))
				 		printf("delete the list\n");
   	  		   	  		close(sockfd);
   	  		  	  		printf("del client\n");
						continue;
   	  		 		 }
					
				  //处理数据函数
				  	write(serial_fd,cliend_buf,sockread_len);
					printf("rcv socket:%s\n",cliend_buf);
				 /* int *temp=(int *)malloc(sizeof(int));
				  *temp=sockfd;
				   pthread_create(&Client_ID,NULL,&ClientThread,temp);
   	  		 	   pthread_detach(&Client_ID);
   	  		 	   */
				}
   	     		}
   		} 
    	}
   return 0;
}
