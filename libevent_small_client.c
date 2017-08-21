#include <sys/types.h>  //基本系统数据类型
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <event.h>
#include <event2/util.h>


int tcp_connect_server(const char *server_ip,int port);

void cmd_msg_cb(int fd,short events,void *arg);

void socket_read_cb(int fd,short events,void *arg);

int main(int argc,char *argv[])
{
	if(argc < 3)
	{
		printf("please input 2 parameter\n");
		return -1;
	}
	//两个参数依次是服务器端的IP地址、端口号
	int sockfd = tcp_connect_server(argv[1],atoi(argv[2]));
	if(sockfd == -1)
	{
		perror("tcp_connect error");
		return -1;
	}
	printf("connect to server successful\n");

	struct event_base* base = event_base_new();//建立一个event_base结构体。可以检测以确认哪个事件是被激活的

	struct event *ev_sockfd = event_new(base,sockfd,EV_READ | EV_PERSIST,socket_read_cb,NULL);//分配并初始化一个新的event结构体，准备被添加
	
	event_add(ev_sockfd,NULL);//把event往当前event中的ev_base追加,如果需要定时，那么第二个参数不能为空

	//监听终端输入事件,STDIN_FILENO表示标准输入
	struct event* ev_cmd = event_new(base,STDIN_FILENO,EV_READ | EV_PERSIST,cmd_msg_cb,(void *)&sockfd);

	event_add(ev_cmd,NULL);

	event_base_dispatch(base);//循环处理事件
	printf("finished \n");
}
void cmd_msg_cb(int fd,short events,void *arg)
{
	char msg[1024];
	int ret = read(fd,msg,sizeof(msg));
	if(ret <= 0){
		perror("read fail");
		exit(1);
	}

	int sockfd = *((int *)arg);

	//把终端的消息发送给服务器
	write(sockfd,msg,ret);
}
void socket_read_cb(int fd,short events,void *arg)
{
	char msg[1024];

	int len = read(fd,msg,sizeof(msg)-1);
	if(len <= 0){
		perror("read fail");
		exit(1);
	}
	msg[len] = '\0';
	printf("recv %s from server\n",msg);
}

typedef struct sockaddr SA;
int tcp_connect_server(const char *server_ip,int port)
{
	int sockfd,status,save_errno;
	struct sockaddr_in server_addr;

	memset(&server_addr,0,sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	status = inet_aton(server_ip,&server_addr.sin_addr);//将一个字符串IP地址转换为一个32位的网络序列IP地址。

	if(status == 0)
	{
		errno = EINVAL;
		printf("EINVAL\n");
		return -1;
	}
	sockfd = socket(PF_INET,SOCK_STREAM,0);
	if(sockfd == -1){
		return sockfd;
	}

	status = connect(sockfd,(SA*)&server_addr,sizeof(server_addr));

	if(status == -1){
		save_errno = errno;
		close(sockfd);
		errno = save_errno;
		return -1;
	}

	evutil_make_socket_nonblocking(sockfd);//设置为非阻塞模式
	return sockfd;
}

