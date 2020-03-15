#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 9990 //端口号
#define SIZE 1024 //定义的数组大小

int Creat_socket() //创建套接字和初始化以及监听函数
{
	int listen_socket = socket(AF_INET, SOCK_STREAM, 0); //创建一个负责监听的套接字
	if (listen_socket == -1)
	{
		perror("socket");
		return -1;
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;				  /* Internet地址族 */
	addr.sin_port = htons(PORT);			  /* 端口号 */
	addr.sin_addr.s_addr = htonl(INADDR_ANY); /* IP地址 */

	int ret = bind(listen_socket, (struct sockaddr *)&addr, sizeof(addr)); //连接
	if (ret == -1)
	{
		perror("Bind");
		return -1;
	}

	ret = listen(listen_socket, 5); //监听
	if (ret == -1)
	{
		perror("listen");
		return -1;
	}
	return listen_socket;
}

int wait_client(int listen_socket)
{
	struct sockaddr_in cliaddr;
	int addrlen = sizeof(cliaddr);
	printf("等待客户端连接。。。。\n");
	int client_socket = accept(listen_socket, (struct sockaddr *)&cliaddr, &addrlen); //创建一个和客户端交流的套接字
	if (client_socket == -1)
	{
		perror("accept");
		return -1;
	}

	printf("成功接收到一个客户端：%s\n", inet_ntoa(cliaddr.sin_addr));

	return client_socket;
}

void hanld_client(int client_socket) //信息处理函数,功能是将客户端传过来的小写字母转化为大写字母
{
	while (1)
	{
		char buf[SIZE] ="";
		int ret = read(client_socket, buf, SIZE-1);
		if (ret == -1)
		{
			perror("read");
			break;
		}
		if (ret == 0)
		{
			break;
		}
        if(strncmp(buf, "q", 1) == 0)
		{
			break;
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
	}
	close(client_socket);
}

int main()
{
	int listen_socket = Creat_socket();

	int client_socket = wait_client(listen_socket);

	hanld_client(client_socket);

     //监听套接字和客户端套接字不等
	close(listen_socket);

	return 0;
}
