#ifndef __COMMON_H__
#define __COMMON_H__

#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888

#define EPOLL_SIZE 5000  /// max epoll size

#define BUF_SIZE 1024 /// max bufsize


#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"


#define SERVER_MESSAGE "[ClientID %d]: %s"
#define SERVER_PRIVATE_MESSAGE "[Client %d] say to you privately: %s"
#define SERVER_PRIVATE_ERROR_MESSAGE "[Client %d] is not in the chat room yet~"

#define EXIT "EXIT"


#define CAUTION "There is only one int the char room!"


struct Msg
{
    int type;
    int fromID;
    int toID;
    char content[BUF_SIZE];

};
#endif // __COMMON_H__

