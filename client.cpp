#include "Common.h"
using namespace std;

void addfd( int epollfd, int fd, bool enable_et )
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if( enable_et )
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0)| O_NONBLOCK);
    printf("fd added to epoll!\n\n");
}


class Client {

public:
    Client();
    void Connect();
    void Close();
    void Start();

private:

    int sock;
    int pid;
    int epfd;
    int pipe_fd[2];
    bool isClientwork;
    Msg msg;
    char send_buf[BUF_SIZE];
    char recv_buf[BUF_SIZE];
    struct sockaddr_in serverAddr;
};

Client::Client(){
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    sock = 0;
    pid = 0;
    isClientwork = true;
    epfd = 0;
}

// 连接服务器
void Client::Connect() {
    printf("Conncet Server %s: %d",SERVER_IP,SERVER_PORT);

    /// socket part;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        perror("sock error");
        exit(-1);
    }

    /// connect part
    if(connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect error");
        exit(-1);
    }

    if(pipe(pipe_fd) < 0) {
        perror("pipe error");
        exit(-1);
    }

    /// epoll create
    epfd = epoll_create(EPOLL_SIZE);

    if(epfd < 0) {
        perror("epfd error");
        exit(-1);
    }

    ///将sock和管道读端描述符都添加到内核事件表中
    addfd(epfd, sock, true);
    addfd(epfd, pipe_fd[0], true);

}


void Client::Close() {

    if(pid){
        close(pipe_fd[0]);
        close(sock);
    }else{
        close(pipe_fd[1]);
    }
}


void Client::Start() {


    static struct epoll_event events[2];

    Connect();

    /// pipe[0] == read   to father
    /// pipe[1] == write  to son
    pid = fork();

    if(pid < 0) {
        perror("fork error");
        close(sock);
        exit(-1);
    } else if(pid == 0) {

        close(pipe_fd[0]);

        printf("Please input 'exit' to exit the chat room\n");
        printf("\\ + ClientID to private chat \n");
        ///如果客户端运行正常则不断读取输入发送给服务端
        while(isClientwork){

            memset(msg.content,0,sizeof(msg.content));
            fgets(msg.content, BUF_SIZE, stdin);
            /// 客户输出exit,退出
            if(strncasecmp(msg.content, EXIT, strlen(EXIT)) == 0){
                isClientwork = 0;
            }
            /// 子进程将信息写入管道
            else {

                bzero(send_buf,BUF_SIZE);
                memcpy(send_buf,&msg,sizeof(msg));
                if( write(pipe_fd[1], send_buf, sizeof(send_buf)) < 0 ) {
                    perror("fork error");
                    exit(-1);
                }
            }
        }
    } else {
        close(pipe_fd[1]);

        while(isClientwork) {
            int epoll_events_count = epoll_wait( epfd, events, 2, -1 );

            for(int i = 0; i < epoll_events_count ; ++i)
            {
                memset(recv_buf,0,sizeof(recv_buf));
                ///服务端发来消息
                if(events[i].data.fd == sock)
                {
                    ///接受服务端广播消息
                    int ret = recv(sock, recv_buf, BUF_SIZE, 0);
                    memset(&msg,0,sizeof(msg));
                    memcpy(&msg,recv_buf,sizeof(msg));

                    if(ret == 0) {

                        printf("Server closed connection: %d\n",sock);
                        close(sock);
                        isClientwork = 0;
                    } else {

                        printf("%s\n",msg.content);
                    }
                }
                else {

                    int ret = read(events[i].data.fd, recv_buf, BUF_SIZE);
                    if(ret == 0)
                        isClientwork = 0;
                    else {
                        send(sock, recv_buf, sizeof(recv_buf), 0);
                    }
                }
            }
        }
    }

    Close();
}

int main(int argc, char *argv[]) {
    Client client;
    client.Start();
    return 0;
}




