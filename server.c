/*
5. 实现一个简单的udp服务器和客户端。有了tcp的经验，udp应该很容易做。
注一：程序：nc发送字符串，服务器接受，并把小写转换为大写，再发送回去(用nc是因为udp)

注二：再自己实现客户端，测试效果


*/

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


void hanld_client(int server_socket) //信息处理函数,功能是将客户端传过来的小写字母转化为大写字母
{
	while (1)
	{
		char buf[SIZE] ="";
		struct sockaddr_in cli_addr;
		int cli_addr_len = sizeof(cli_addr);

		int ret = recvfrom(server_socket, buf, SIZE-1, 0, (struct sockaddr*)&cli_addr, &cli_addr_len);
		if (ret == -1)
		{
			perror("recvfrom");
			break;
		}
		if (ret == 0)
		{
			break;
		}

		//这里的减二是经过测试，发现telnet每次都会多发两个无用字符作为结束标
		//经wireshark抓包工具分析：linux telnet发送报文 然后以回车键结束,发送的报文中末尾 默认会加入\r\n
		buf[ret] = '\0';
        printf("Receive info: %s from %s:%d\n", buf,inet_ntoa(cli_addr.sin_addr),cli_addr.sin_port);

		int i;

		for (i = 0; i < ret; i++)
		{
			buf[i] = buf[i] + 'A' - 'a';
		}		
		sendto(server_socket, buf, ret+1, 0, (struct sockaddr*)&cli_addr, cli_addr_len);
		if(strncmp(buf, "END", 3) == 0)
		{
			break;
		}
	}
}

int main()
{
	char buf[SIZE];
	int server_socket = Creat_socket();

	printf("等待客户端连接。。。。\n");

	hanld_client(server_socket);

     //监听套接字和客户端套接字不等
	close(server_socket);

	return 0;
}
