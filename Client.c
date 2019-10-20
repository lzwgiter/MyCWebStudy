/*************************************************************************
	> File Name: Client.c
	> Author: flo@t
	> Mail: float311@163.com
	> Created Time: 2019年10月18日
	> Description:
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>

# define portnumber 9999			//服务端服务程序的端口号

int main(void) {
	int nbytes;
	int sockfd;
	char buffer[80];
	char buffer_2[80];
	struct sockaddr_in server_addr;

	/*
	 * 调用socket函数创建一个TCP协议套接字
	 */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		fprintf(stderr, "Socket error:  %s\n", strerror(errno));
		exit(1);
	}

	bzero(&server_addr, sizeof(struct sockaddr_in));
	server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portnumber);

	if (connect(sockfd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) == -1){
		fprintf(stderr, "Connect error:  %s\n", strerror(errno));
		exit(1);
	}

	while(1)
	{
		printf("\033[33mPlease Input Strings to transfer: \n");
		fgets(buffer, 1024, stdin);
		printf("\033[33mYour Input : %s", buffer);
		write(sockfd, buffer, strlen(buffer));
		if ((nbytes = read(sockfd, buffer_2, 81)) == -1){
			fprintf(stderr, "Read error:  %s\n", strerror(errno));
			exit(1);
		}
		buffer_2[strlen(buffer_2)] = '\0';
		printf("Message From Server : %s\n", buffer_2);
	}
	close(sockfd);
	exit(0);
}
