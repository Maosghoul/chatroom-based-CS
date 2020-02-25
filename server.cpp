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

class Server {

public:
    Server();
    void Init();
    void Close();
    void Start();

private:
    int SendBroadcastMessage(int clientfd);
    struct sockaddr_in serverAddr;
    int listenfd;
    int epfd;
    list<int> clients_list;
};


Server::Server(){

    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    listenfd = 0;
    epfd = 0;
}
void Server::Init() {

    printf("Init Server.....\n");
    /// socket part
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) { perror("socket error"); exit(-1);}

    /// bind part
    if( bind(listenfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind error");
        exit(-1);
    }

    /// listen part
    int ret = listen(listenfd, 5);
    if(ret < 0) {
        perror("listen error");
        exit(-1);
    }
    printf("Start to listen:%s\n",SERVER_IP);

    epfd = epoll_create (EPOLL_SIZE);

    if(epfd < 0) {
        perror("epfd error");
        exit(-1);
    }

    /// add epfd and listenfd  to epoll
    addfd(epfd, listenfd, true);

}

void Server::Close() {

    close(listenfd);
    close(epfd);
}

/// broadcast
int Server::SendBroadcastMessage(int clientfd)
{
    char recv_buf[BUF_SIZE];  /// receive buffer
    char send_buf[BUF_SIZE];  /// send buffer
    Msg msg;
    bzero(recv_buf, BUF_SIZE);

    printf("read from [client%d]\n",clientfd);

    int len = recv(clientfd, recv_buf, BUF_SIZE, 0);

    memset(&msg,0,sizeof(msg));
    memcpy(&msg,recv_buf,sizeof(msg));
    ///判断是私聊还是群聊
    msg.fromID=clientfd;
    if(msg.content[0]=='\\'&&isdigit(msg.content[1])){
        msg.type=1;
        msg.toID=msg.content[1]-'0';
        memcpy(msg.content,msg.content+2,sizeof(msg.content));
    }
    else
        msg.type=0;
    if(len == 0) /// biao shi   client is closed
    {
        close(clientfd);
        clients_list.remove(clientfd);
        printf("[Client %d ]clised.\n now there are %d clients in the charroom\n",clientfd,(int)clients_list.size());

    }
    else
    {
        if(clients_list.size() == 1){
            memcpy(&msg.content,CAUTION,sizeof(msg.content));
            bzero(send_buf, BUF_SIZE);
            memcpy(send_buf,&msg,sizeof(msg));
            send(clientfd, send_buf, sizeof(send_buf), 0);
            return len;
        }
        char format_message[BUF_SIZE];
        if(msg.type==0){
            /// 格式化发送的消息内容
            sprintf(format_message, SERVER_MESSAGE, clientfd, msg.content);
            memcpy(msg.content,format_message,BUF_SIZE);
            /// 遍历客户端列表依次发送消息，需要判断不要给来源客户端发

            for(int &p : clients_list){
                if(p != clientfd){
                    bzero(send_buf,BUF_SIZE);
                    memcpy(send_buf,&msg,sizeof(msg));
                    if(send(p,send_buf,sizeof(send_buf),0)<0){
                        return -1;
                    }
                }
            }
        }
        if(msg.type==1){
            bool private_offline=true;
            sprintf(format_message, SERVER_PRIVATE_MESSAGE, clientfd, msg.content);
            memcpy(msg.content,format_message,BUF_SIZE);
            /// 遍历客户端列表依次发送消息，需要判断不要给来源客户端发

            for(int &p : clients_list){
                if(p==msg.toID){
                    private_offline = false;
                    bzero(send_buf,BUF_SIZE);
                    memcpy(send_buf,&msg,sizeof(msg));
                    if(send(p,send_buf,sizeof(send_buf),0)<0){
                        return -1;
                    }
                }
            }

            if(private_offline){
                sprintf(format_message,SERVER_PRIVATE_ERROR_MESSAGE,msg.toID);
                memcpy(msg.content,format_message,BUF_SIZE);
                bzero(send_buf,BUF_SIZE);
                memcpy(send_buf,&msg,sizeof(msg));
                if(send(msg.fromID,send_buf,sizeof(send_buf),0)<0)
                    return -1;
            }
        }
    }
    return len;
}


void Server::Start() {

    static struct epoll_event events[EPOLL_SIZE];

    /// part reuse
    int opt = 1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    Init();

    while(1)
    {
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);

        if(epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }

        printf("epoll_events_count %d=\n",epoll_events_count);


        for(int i = 0; i < epoll_events_count; ++i)
        {
            int sockfd = events[i].data.fd;
            /// new user in
            if(sockfd == listenfd)
            {
                /// accept part
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrLength );

                printf("ClientID = %d clised.\n now there are %d clients in the charroom\n",clientfd,(int)clients_list.size());

                addfd(epfd, clientfd, true);

                clients_list.push_back(clientfd);
                printf("Add new Clientfd %d to epoll\n",clientfd);
                printf("Now there are %d clients in the chatroom\n",(int)clients_list.size());
                printf("welcome message\n");

                char message[BUF_SIZE];
                bzero(message, BUF_SIZE);
                sprintf(message, SERVER_WELCOME, clientfd);
                int ret = send(clientfd, message, BUF_SIZE, 0);
                if(ret < 0) {
                    perror("send error");
                    Close();
                    exit(-1);
                }
            }
            else {
                int ret = SendBroadcastMessage(sockfd);
                if(ret < 0) {
                    perror("error");
                    Close();
                    exit(-1);
                }
            }
        }
    }

    Close();
}



int main(int argc, char *argv[]) {
    Server server;
    server.Start();
    return 0;
}


