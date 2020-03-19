/*
1. 4. 改写第一步的简单服务器，用select的方式，单进程条件下，实现多客户端的支持。调试这个程序
注：程序：telnet发送字符串，服务器接受，并把小写转换为大写，再发送回去

遇到的问题一：select函数实现单进程服务器服务多个客户端
遇到的问题二：iotcl函数的使用

*/

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>    /* BSD and Linux */

#define PORT 9990 //端口号
#define SIZE 1024 //定义的数组大小

int Creat_tcpSocket() //创建套接字和初始化以及监听函数
{
	int server_tcpSocket = socket(AF_INET, SOCK_STREAM, 0); //创建一个负责监听的套接字
	if (server_tcpSocket == -1)
	{
		perror("socket");
		return -1;
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;				  /* Internet地址族 */
	addr.sin_port = htons(PORT);			  /* 端口号 */
	addr.sin_addr.s_addr = htonl(INADDR_ANY); /* IP地址 */

	int ret = bind(server_tcpSocket, (struct sockaddr *)&addr, sizeof(addr)); //连接
	if (ret == -1)
	{
		perror("Bind");
		exit(1);
	}

	ret = listen(server_tcpSocket, 5); //监听
	if (ret == -1)
	{
		perror("listen");
		return -1;
	}
	return server_tcpSocket;
}

int Creat_udpsocket() //创建套接字和初始化以及监听函数
{
	int server_socket = socket(AF_INET, SOCK_DGRAM, 0); //创建一个负责监听的套接字
	if (server_socket == -1)
	{
		perror("socket");
		return -1;
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;				  /* Internet地址族 */
	addr.sin_port = htons(PORT);			  /* 端口号 */
	addr.sin_addr.s_addr = htonl(INADDR_ANY); /* IP地址 */

	int ret = bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)); //连接
	if (ret == -1)
	{
		perror("Bind");
		return -1;
	}
	return server_socket;
}
void hanld_tcpclient(int client_socket, fd_set*readfds) //信息处理函数,功能是将客户端传过来的小写字母转化为大写字母
{
	char buf[SIZE] ="";
	int ret = read(client_socket, buf, SIZE-1);
	if (ret == -1)
	{
		perror("read");
	}
	//这里的减二是经过测试，发现telnet每次都会多发两个无用字符作为结束标
	//经wireshark抓包工具分析：linux telnet发送报文 然后以回车键结束,发送的报文中末尾 默认会加入\r\n
	buf[ret-1] = '\n';
	buf[ret] = '\0';
	printf("%s", buf);
	int i;
	for (i = 0; i < (ret-2); i++)
	{
		buf[i] = buf[i] + 'A' - 'a';
	}		
	write(client_socket, buf, ret);
	if(strncmp(buf, "END", 3) == 0)
	{
		FD_CLR(client_socket, readfds);
		close(client_socket);
	    printf("成功关闭一个客户端\n"); 
	}
}

void hanld_udpclient(int server_socket) //信息处理函数,功能是将客户端传过来的小写字母转化为大写字母
{
	char buf[SIZE] ="";
	struct sockaddr_in cli_addr;
	int cli_addr_len = sizeof(cli_addr);

	int ret = recvfrom(server_socket, buf, SIZE-1, 0, (struct sockaddr*)&cli_addr, &cli_addr_len);
	if (ret == -1)
	{
		perror("recvfrom");
		exit(1);
	}

	buf[ret] = '\0';
    printf("Receive info: %s from %s:%d\n", buf,inet_ntoa(cli_addr.sin_addr),cli_addr.sin_port);

	int i;

	for (i = 0; i < ret; i++)
	{
		buf[i] = buf[i] + 'A' - 'a';
	}		
	sendto(server_socket, buf, ret+1, 0, (struct sockaddr*)&cli_addr, cli_addr_len);
}
int main()
{
	int server_tcpSocket = Creat_tcpSocket(), client_sockfd;
	int server_udpSocket = Creat_udpsocket();

	int result;
	fd_set readfds, testfds;

	FD_ZERO(&readfds);
	//将服务器端socket加入到集合中
	FD_SET(server_tcpSocket,&readfds);
	FD_SET(server_udpSocket,&readfds);
	printf("等待客户端连接。。。。\n");
	while(1)
	{
		int fd;
		int nread;
		testfds = readfds;		
		//不阻塞，并测试是否有客户端尝试连接
		result = select(FD_SETSIZE, &testfds, (fd_set*)0, (fd_set*)0, (struct  timeval*)0);
		if(result < 1)
		{
			perror("server");
		}
		else
		{
			/*扫描所有的文件描述符*/
			for(fd = 0; fd < FD_SETSIZE; fd++) 
			{
				/*找到相关文件描述符*/
				if(FD_ISSET(fd,&testfds)) 
				{ 
			     	/*判断是否为服务器套接字，是则表示为客户请求连接。*/
					if(fd == server_tcpSocket) 
					{ 
						struct sockaddr_in client_address;
						int addrlen = sizeof(client_address);
						client_sockfd = accept(server_tcpSocket, (struct sockaddr *)&client_address, &addrlen); 
						FD_SET(client_sockfd, &readfds);//将客户端socket加入到集合中
						printf("成功接收到一个客户端：%s\n", inet_ntoa(client_address.sin_addr)); 
					}
					else if(fd == server_udpSocket)
					{
						hanld_udpclient(server_udpSocket);
					} 
					/*客户端socket中有数据请求时*/
					else 
					{                                          
						ioctl(fd, FIONREAD, &nread);//取得数据量交给nread
						
						/*客户数据请求完毕，关闭套接字，从集合中清除相应描述符 */
						//可以读，但数据为0，说明这是一个结束消息
						if(nread == 0) 
						{ 
							FD_CLR(fd, &readfds); 
							close(fd); 
							printf("成功关闭一个客户端\n"); 
						} 
						/*处理客户数据请求*/
						else 
						{ 
							hanld_tcpclient(fd, &readfds);
						} 
					} 
				} 
			}
	    }
	}
     //监听套接字和客户端套接字不等
	close(server_tcpSocket);
	close(server_udpSocket);
	return 0;
}
