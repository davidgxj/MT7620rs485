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


char cliend_buf[128];//�������ݻ������SOCKET����
int sockread_len;//�ͻ��˶�ȡ���ֽ���

char ipaddr[16];//���ڴ�Ż�ȡ��Ip��ַ
char dns_addr[50];//���ڴ�Ŵ��Զ��������ַ
int ser_port;//Զ�̶˿�
int client_port;//���ض˿�
char RS232_BUF;//���ڶ˿�
int RS232_BAUD;//���ڲ�����

int serial_fd;//����FD


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
	/* ���տͻ��˷��������ݲ���ʾ���� */
		
		if (sockread_len <= 0)
		{
			close(rev_fd);
			pthread_exit(NULL);
		}
		else
		{
			//д��ѡ�еĴ�����
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
  //�����߳�
  signal(SIGCHLD,handle_sigchld);
  createNodelist();  //�����������ڴ洢FD
  //���ڳ�ʼ����ȡ·��ֵ���ô����Լ�Զ�����Ӷ˿�
  GetConfigValue("/etc/config/rs232" );
  
  printf("client ipport is %d\n",client_port);//���ض˿�
  printf("server ipport is %d\n",ser_port);//Զ�̶˿�
  printf("the server ip is %s\n",ipaddr);

  
   /*����epoll_event�ṹ��ı�����ev����ע���¼���events
   �������ڻش�Ҫ������¼�*/
   
   struct epoll_event ev,events[20];//���������ڻش����յ��¼�
   /*�������ڴ���accept��epollר��
   ���ļ���������ָ�����������������ΧΪ256*/
  int epoll_instance=epoll_create(200);//size is unused nowadays

  int  listenfd = socket(AF_INET,SOCK_STREAM,0);
   if(listenfd <0)
   {
      perror("error opening socket");
      return -1;
   }
   //setnonblocking(listenfd);//�����ڼ�����socket���óɷ�������ʽ
   ev.data.fd=listenfd;//������Ҫ������¼���ص��ļ�������
   ev.events=EPOLLIN|EPOLLET;//����Ҫ������¼�����
   epoll_ctl(epoll_instance,EPOLL_CTL_ADD,listenfd,&ev);//ע��epoll�¼�
/*
���ط��������ã��󶨶˿ڣ�����ǿͻ���ֱ�Ӷ�ȡԶ�̵�ip
*/
   struct sockaddr_in client_addr;
   memset(&client_addr,0,sizeof(client_addr));
   client_addr.sin_family = AF_INET;
   client_addr.sin_addr.s_addr = INADDR_ANY;
   client_addr.sin_port = htons((uint16_t )client_port);//���ض˿�
/*����˿ڸ�������*/
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

/*����Զ������
*/
struct timeval  timeout = {3,0};
socklen_t len_time = sizeof(timeout);

//����Զ��SOCKET
  int  ser_lister = socket(AF_INET,SOCK_STREAM,0);
  setsockopt(ser_lister, SOL_SOCKET, SO_SNDTIMEO, &timeout,len_time);
  if(ser_lister<0)
  	{
  		perror("error opening server_socket\n");
     		return -1;
  	}
  //Զ�̷�����
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
  		//���ö˿�
  		ser_addr.sin_port = htons((uint16_t )ser_port);
   		/*���ö˿ڸ���*/
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
		  /*��Զ����������epoll �¼�����*/
   		ev.data.fd=ser_lister ;//������Ҫ������¼���ص��ļ�������
   		ev.events=EPOLLIN|EPOLLET;//����Ҫ������¼�����
   		epoll_ctl(epoll_instance,EPOLL_CTL_ADD,ser_lister ,&ev);//ע��epoll�¼�
   		   /*���½���SERVER�ڵ����������*/
  		Node * sernode=(Node*)malloc(sizeof(Node));
  		sernode->fd_data=ser_lister;
  		sernode->pNext=NULL;
 		addNode(sernode);
	}
//�����豸�ļ�
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
			//�������¼����뵽epoll ��
			ev.data.fd=serial_fd;//������Ҫ������¼���ص��ļ�������
   			ev.events=EPOLLIN|EPOLLET;//����Ҫ������¼����Ͷ������ش���
   			epoll_ctl(epoll_instance,EPOLL_CTL_ADD,serial_fd,&ev);//ע��epoll�¼�
			printf("serial_fd is %d\n",serial_fd);	
			}
		}
   while(1)
   {
    int nfound=epoll_wait(epoll_instance,events,20,1000);//�ȴ�epoll�¼��ķ���,�����¼���
   	  if(nfound==0)
   	  {
   	  	//printf(".");
   	  	fflush(stdout);
		continue;
   	  }
	int n_poll;
    for(n_poll=0;n_poll<nfound;n_poll++)
   	  {
   	  	if(events[n_poll].data.fd==listenfd)/*���ض˿����µ�����*/
   	  	{
   	  		struct sockaddr_in cliaddr;//���ӵ����صĿͻ���
   	  		uint32_t  len = sizeof(cliaddr);
   	  		int connfd=accept(listenfd,(struct sockaddr*)&cliaddr,&len);
   	  		printf("connection from host %s,port %d,sockfd is %d\n",
             		inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port),connfd);
   	  		  //setnonblocking(connfd);
   	  		  
   	  		  ev.data.fd=connfd;//�������ڶ��������ļ�������
   	  		  ev.events=EPOLLIN|EPOLLET;//��������ע��Ķ������¼�
   	  		  epoll_ctl(epoll_instance,EPOLL_CTL_ADD,connfd,&ev);//ע��ev�¼�
			  printf("add %d to epoll\n",connfd);
			  
			//info_data.socfd=connfd;
			//printf("connect is %d\n,info_data.socfd");
			//���ϵͳ��Ϣ
			//��ӵ�����
			Node * clientnode=(Node*)malloc(sizeof(Node));
			clientnode->fd_data=connfd;
			clientnode->pNext=NULL;
			addNode(clientnode);
   	  	}
   	  	else if(events[n_poll].events&EPOLLIN)/*���¼�*/
   	  		{    
   	  			if(events[n_poll].data.fd==serial_fd)
   	  			{
   	  				int read_cont;//��ȡ�����ڵ��ֽ���
					char serial_buf[128];//���ڴ�Ŵ��ڵ�����
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
    						   	ev.data.fd=ser_lister;//�������ڶ��������ļ�������
   	  		  				ev.events=EPOLLIN|EPOLLET;//��������ע��Ķ������¼�
   	  		  				epoll_ctl(epoll_instance,EPOLL_CTL_DEL,ser_lister,&ev);//ɾ���¼�
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
					//�ж��Ƿ���SOCKET�¼�
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
					
				  //�������ݺ���
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
