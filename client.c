#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
 
 
#define PORT 9990
#define SIZE 1024
 
int main()
{
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);   //创建和服务器连接套接字
    if(client_socket == -1)
    {
        perror("socket");
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    
    addr.sin_family = AF_INET;  /* Internet地址族 */
    addr.sin_port = htons(PORT);  /* 端口号 */
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   /* IP地址 */
    inet_aton("127.0.0.1", &(addr.sin_addr));
 
    int addrlen = sizeof(addr);

    char buf[SIZE];
    
    while(1)        //向服务器发送数据，并接收服务器转换后的大写字母
    {
        printf("请输入你相输入的：");
        scanf("%s", buf);
        int count = sendto(client_socket, buf, strlen(buf), 0, (struct sockaddr*)&addr, addrlen);

        int ret = recvfrom(client_socket,buf,sizeof(buf),0,(struct sockaddr*)&addr,&addrlen);
        if (ret==-1)
        {
            printf("receive data failure\n");
            return -1;
        }
        buf[ret] = '\0';
        printf("ret = %d\nReceive info: %s\n", ret,buf);
        if(!strcmp(buf, "end"))
            break;
    }
    close(client_socket);
    
    return 0;
}

