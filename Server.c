/*************************************************************************
 > File Name: server.c
 > Author: flo@t
 > Mail: float311@163.com
 > Created Time: 2019年10月18日
 > Description:
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>

/*
 * 宏定义端口号
 */
#define portnumber 9999
#define MAX_LINE 80
#define MAX_LISTEN 5

struct sockaddr_in cli;		//客户端结构
struct sockaddr_in server;		//服务端结构
int client[MAX_LISTEN];		//客户端链接的套接字描述数组
int maxfd;										//最大文件描述符
fd_set rset;									//可读文件描述符集合
fd_set aset;									//活动文件描述符集合，用于保存所有文件描述符
/*处理函数，将大写字符转换为小写字符，参数为需要转换的字符串*/
void my_fun(char *p)
{
	//空串
	if (p == NULL)
	{
		return;
	}

	//判断字符，并进行转换
	for (; *p != '\0'; p++)
	{
		if (*p >= 'A' && *p <= 'Z')
		{
			*p = *p - 'A' + 'a';
		}
	}
}

void check_online()
{
	int i;
	int tmpfd;
	for (i = 0; i < MAX_LISTEN; ++i)
	{
		tmpfd = client[i];
		if (!FD_ISSET(tmpfd, &aset))
		{
			FD_CLR(tmpfd, &aset);		//从集合中删除该用户
			close(tmpfd);		//关闭与该用户套接字的连接
			client[i] = -1;		//复位该位置的标识符
		}
	}
}

void accept_client_proc(int lfd)
{
	socklen_t addr_len;
	int cfd;		//Clientfd
	int i;
	addr_len = sizeof(server);
	/*接受客户端的请求*/
	if ((cfd = accept(lfd, (struct sockaddr*) (&cli), &addr_len)) == -1)
	{
		fprintf(stderr, "Accept error : %s\n", strerror(errno));
		exit(1);
	}

	FD_SET(cfd, &aset);		//客户端加入全部的活动集合
	maxfd = (cfd > maxfd) ? cfd : maxfd;		//更新当前的最大文件描述符

	/*打印客户端地址和端口号*/
	printf("Connected from %s : %d\n", inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));

	/*查找一个空闲位置*/
	for (i = 0; i < MAX_LISTEN; ++i)
	{
		if (client[i] <= 0)
		{
			client[i] = cfd;			//将客户端的文件描述符放入该位置
			break;
		}
	}

	/*如果有超过MAX_LISTEN的请求数量，服务器关闭*/
	if (i == MAX_LISTEN)
	{
		printf("Too Many Client! Server shutdown...\n");
		exit(1);
	}

	/*test*/
	for (int x = 0; x < MAX_LISTEN; ++x) {
		printf("%d ", client[x]);
	}
	printf("\n");
}

void transfer_proc()
{
	int i, n;
	int tmpfd;
	char buffer[MAX_LINE];
	memset(buffer, 0, sizeof(buffer));

	/*判断哪一个套接字已经准备就绪*/
	for (i = 0; i < maxfd; ++i)
	{
		if ((tmpfd = client[i]) < 0)
		{
			continue;
		}
		if (FD_ISSET(tmpfd, &rset))				//在保持客户端连接的条件下
		{
			printf("Using Users'fd %d\n", tmpfd);
			n = read(tmpfd, buffer, MAX_LINE);		//接收客户端发送的要进行变换的字符串
			if (n)
			{
				printf("User's Input : %s", buffer);

				/*调用大小写转换函数*/
				my_fun(buffer);
				printf("Send to fd %d : %s\n", tmpfd, buffer);
				send(tmpfd, buffer, sizeof(buffer), 0);
			}
			else			//用户退出程序
			{
				FD_CLR(tmpfd, &aset);
				check_online();
				close(tmpfd);
			}
		}
	}
}

int server_init()
{
	int lfd;			//Server-listen-fd
	int opt = 1;		//套接字选项
	int i;

	/*
	 * 对server_addr_in结构进行赋值
	 */
	memset(&server, 0, sizeof(struct sockaddr_in));		//清零
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(portnumber);
	server.sin_family = AF_INET;

	/*调用socket函数创建一个TCP协议套接字*/
	if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "Socket error : %s\n", strerror(errno));
		exit(1);
	}

	/*设置套接字选项，使用默认的配置, SO_REUSEADDR可以让端口释放后立即使用*/
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	/*调用bind函数将当前的socket与主机地址进行绑定*/
	if (bind(lfd, (struct sockaddr*) &server, sizeof(struct sockaddr)) == -1)
	{
		fprintf(stderr, "Bind error : %s\n", strerror(errno));
		exit(1);
	}

	/*初始化客户端连接描述符集合*/
	for (i = 0; i < MAX_LISTEN; ++i)
	{
		client[i] = -1;
	}

	printf("Server start ..... ok\n");

	/*
	 * 开始监听端口，连接客户端
	 */
	if (listen(lfd, MAX_LISTEN) == -1)
	{
		fprintf(stderr, "Listen error : %s\n", strerror(errno));
		exit(1);
	}
	printf("Waiting for Connections......\n");

	maxfd = lfd;
	return lfd;
}

int main(void)
{
	int lfd;			//Server-listen-fd

	lfd = server_init();

	FD_ZERO(&aset);
	FD_SET(lfd, &aset);			//加入监听字
	/*开始服务的死循环*/
	while (1)
	{
		memcpy(&rset, &aset, sizeof(rset));		//将aset中的套接字描述符添加都rset中
		/*得到当前可以读的文件描述符数*/
		select(maxfd + 1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(lfd, &rset))			//判断主监听套接字是否已经准备就绪
		{
			accept_client_proc(lfd);
		}
		else				//当主监听套接字没有就绪时，则测试其他监听字的状态
		{
			transfer_proc();
		}
	}
	/*
	close(lfd);
	FD_CLR(lfd, &aset);
	return 0;*/
}
