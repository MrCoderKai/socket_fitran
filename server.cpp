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

constexpr int MAX_SOCKET_PACKAGE_LEN = 1024;
constexpr int MAX_CLIENT_NUM = 20;
constexpr int SERVER_PORT = 24639;
std::string SERVER_IP_ADDRESS = "39.107.75.198";
int g_Socket;
int g_AcceptSocket;
struct sockaddr_in g_ServerSockAddr;
bool g_ExitFlag = false;
bool g_acceptThreadExitFlag = false;

struct sockaddr_in g_ClentAddrArray[MAX_CLIENT_NUM];

void initial()
{
    memset(&g_ClentAddrArray, 0, sizeof(g_ClentAddrArray));
}


bool notifyAcceptTheadExit()
{
    while(!g_acceptThreadExitFlag)
    {
        if(!g_ExitFlag)
            continue;
        int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(conn_fd < 0)
        {
            std::cout << "notifyAcceptTheadExit: Create Socket Failed." << std::endl;
        }
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof (struct sockaddr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_PORT);
        serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS.c_str());
        
        // send connect request to server.
        if (connect(conn_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0)
        {
           std::cout << "notifyAcceptTheadExit: Connect Failed." << std::endl;
        }
        else
        {
           std::cout << "notifyAcceptTheadExit: Connect Sucess." << std::endl;
        }
        usleep(200000);
    }
    std::cout << "notifyAcceptTheadExit Thead Exit." << std::endl;
}


bool isStop()
{
    while(1)
    {
        if(access("stop", F_OK) == 0)
        {
            g_ExitFlag = true;
            break;
        }
    }
    std::cout << "isStop Thead Exit." << std::endl;
}


void acceptThread()
{
    struct sockaddr_in t_ClientAddr;
    socklen_t t_ClientLen = sizeof (struct sockaddr_in);
    while(!g_ExitFlag)
    {
        memset(&t_ClientAddr, 0, sizeof (struct sockaddr_in));
        g_AcceptSocket = accept(g_Socket, (struct sockaddr *)&t_ClientAddr, &t_ClientLen);
        if(g_AcceptSocket < 0)
        {
            std::cout << "acceptThread: Accept Failed" << std::endl;
            break;
        }
        else
        {
            std::cout << "acceptThread: Accept a new client, ip: " << inet_ntoa(t_ClientAddr.sin_addr) << std::endl;
        }
    }
    std::cout << "Accept Tread Exits." << std::endl;
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
    // g_ServerSockAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS.c_str());
    g_ServerSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    g_ServerSockAddr.sin_port = htons(SERVER_PORT);
    std::cout << "Hello, this is socket programming demo." << std::endl;
    if (bind(g_Socket, (struct sockaddr *)&g_ServerSockAddr, sizeof(struct sockaddr)) < 0)
    {
        std::cout << "Bind Failed." << std::endl;
        close(g_Socket);
        return 0;
    }
    else
    {
        std::cout << "Bind Success." << std::endl;
    }
    if(listen(g_Socket, 1024) < 0)
    {
        std::cout << "Listen Error." << std::endl;
        close(g_Socket);
        return 0;
    }
    else
    {
        std::cout << "Listen Success." << std::endl;
    }
    // Create Thread
    std::thread t_AcceptThread(acceptThread);
    std::thread t_isStopThread(isStop);
    std::thread t_notifyAcceptTheadExit(notifyAcceptTheadExit);
    // int ret = pthread_create(&t_AcceptThread, NULL, acceptThread, NULL);
    t_AcceptThread.join();
    t_isStopThread.join();
    t_notifyAcceptTheadExit.join();
    close(g_Socket);
    std::cout << "Exit." << std::endl;

    return 0;
}

