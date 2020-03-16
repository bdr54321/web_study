/*
3. 改写第一步的简单服务器，采用fork方式，多进程条件下，实现多客户端的支持。
注：程序：telnet发送字符串，服务器接受，并把小写转换为大写，再发送回去。
          服务器多进程
          
问题记录
1.僵死进程的处理
解决：  见handler函数
  
2.在客户端输入end后，服务器端对应子进程终止，但客户端却处于半关闭状态
解决：在每个对应进程，都需要先把不再用的文件描述符关闭，因为父子进程共享套接字描述符，并且采用引用计数

3.系统函数signal和waitpid和僵死进程

*/

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

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
		//客户端主动终止
		if (ret == 0)
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
		//客户端发送end，服务器端先终止(这里由于用telnet做客户端，只能用服务器终止了，每次终止后都得停一会才能再次运行)
		if(strncmp(buf, "END", 3) == 0)
		{
			break;
		}
		
	}
	close(client_socket);
}

void handler(int sig)
{
	//循环的过程就是留时间给父进程处理僵死进程
	while (waitpid(-1,  NULL,   WNOHANG) > 0)
	{
		printf ("成功处理一个子进程的退出\n");
	}
}

int main()
{
	int listen_socket = Creat_socket();
	
    //SIGCHLD就是内核在任何一个进程终止时发送给父进程的一个信号。
	signal(SIGCHLD,  handler);    //处理子进程，防止僵尸进程的产生
	while(1)
	{
		int client_socket = wait_client(listen_socket);   //多进程服务器，可以创建子进程来处理，父进程负责监听。
		
	    // 1）在父进程中，fork返回新创建子进程的进程ID；
        // 2）在子进程中，fork返回0；
        // 3）如果出现错误，fork返回一个负值；
		int pid = fork();
		if(pid == -1)
		{
			perror("fork");
			break;
		}
		//位于父进程
		if(pid > 0)
		{					
		    close(client_socket);//释放不用资源
			continue;
		}
		//位于子进程
		if(pid == 0)
		{
			close(listen_socket);//释放不用资源
			hanld_client(client_socket);
			close(client_socket);
			break;
		}
	}
	return 0;

}
