#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include "common.h"

//constexpr int MAX_SOCKET_PACKAGE_LEN = 1024;
//constexpr int MAX_CLIENT_NUM = 20;
//constexpr int SERVER_PORT = 24644;

std::mutex g_mutex;

// std::string SERVER_IP_ADDRESS = "39.107.75.198";
//std::string SERVER_IP_ADDRESS = "127.0.0.1";
int g_Socket;
int g_AcceptSocket;
struct sockaddr_in g_ServerSockAddr;

void initial()
{
}

void sendThread()
{
    usleep(20000);
    std::cout << "Hello, This is send thread" << std::endl;
    usleep(2000000);
    g_mutex.lock();
    std::cout << "Send Thread Exits." << std::endl;
    g_mutex.unlock();
}


void recvThread()
{
    usleep(20000);
    std::cout << "Hello, This is receive thread" << std::endl;
    char t_recvBuf[1024];
    std::cout << "Begin to receive ..." << std::endl;
    int cnt = 0;
    std::cout << "Before receive, cnt = " << cnt << std::endl;
    cnt = (int)recv(g_Socket, t_recvBuf, 1024, 0 );
    std::cout << "After receive, cnt = " << cnt << std::endl;
    usleep(20);
    std::cout << t_recvBuf << std::endl;
    usleep(2000000);
    g_mutex.lock();
    std::cout << "Receive Thread Exits." << std::endl;
    g_mutex.unlock();
}


int main(int argc, char** argv)
{
    initial();
    // pthread_t t_AcceptThread;
    g_Socket = socket(PF_INET, SOCK_STREAM, 0);
    int optVal = 20 * 1024 * 1024;
    int optLen = sizeof(int);
    setsockopt(g_Socket, SOL_SOCKET, SO_RCVBUF, (char*)&optVal, optLen);
    setsockopt(g_Socket, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, optLen);
    memset(&g_ServerSockAddr, 0, sizeof (struct sockaddr_in));
    g_ServerSockAddr.sin_family = PF_INET;
    g_ServerSockAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS.c_str());
    g_ServerSockAddr.sin_port = htons(SERVER_PORT);
    std::cout << "Hello, this is Client socket programming demo." << std::endl;
    if (connect(g_Socket, (struct sockaddr *)&g_ServerSockAddr, sizeof(struct sockaddr)) < 0)
    {
        std::cout << "Connect Failed." << std::endl;
        close(g_Socket);
        return 0;
    }
    else
    {
        std::cout << "Connect Success." << std::endl;
    }
    // Create Thread
    std::thread t_tSendThread(sendThread);
    std::thread t_tRecvThread(recvThread);
    usleep(2000);
    t_tSendThread.join();
    t_tRecvThread.join();
    usleep(2000);
    close(g_Socket);
    std::cout << "Main Thread Exit." << std::endl;

    return 0;
}

