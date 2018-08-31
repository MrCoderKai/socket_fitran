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

constexpr int MAX_SOCKET_PACKAGE_LEN = 1024;
constexpr int MAX_CLIENT_NUM = 20;
constexpr int SERVER_PORT = 24640;
int g_ClientSocketFd[MAX_CLIENT_NUM];
bool g_ClientSocketAliveStatus[MAX_CLIENT_NUM];
struct sockaddr_in g_ClentAddrArray[MAX_CLIENT_NUM];
bool g_bSendThreadIdleFlag[MAX_CLIENT_NUM];
std::thread* g_ptSendThread[MAX_CLIENT_NUM];
bool g_bRecvThreadIdleFlag[MAX_CLIENT_NUM];
std::thread* g_ptRecvThread[MAX_CLIENT_NUM];

std::mutex g_mutex;

std::string SERVER_IP_ADDRESS = "39.107.75.198";
int g_Socket;
int g_AcceptSocket;
struct sockaddr_in g_ServerSockAddr;
bool g_ExitFlag = false;
bool g_acceptThreadExitFlag = false;

void initial()
{
    memset(&g_ClentAddrArray, 0, sizeof(g_ClentAddrArray));
    memset(&g_ClientSocketAliveStatus, 0, sizeof(g_ClientSocketAliveStatus));
    memset(&g_ptSendThread, 0, sizeof(g_ptSendThread));
    memset(&g_ptRecvThread, 0, sizeof(g_ptRecvThread));
    for(int i=0; i < MAX_CLIENT_NUM; ++i)
    {
        g_ClientSocketFd[i] = -1;
        g_bSendThreadIdleFlag[i] = true;
        g_bRecvThreadIdleFlag[i] = true;
    }
}


int findIdleSocketIdx()
{
    g_mutex.lock();
    for(int i=0; i < MAX_CLIENT_NUM; ++i)
    {
        if(!g_ClientSocketAliveStatus[i]){
            g_mutex.unlock();
            return i;
        }
    }
    g_mutex.unlock();
    return -1;
}


bool notifyAcceptTheadExit()
{
    usleep(20000);
    std::cout << "Hello, This is notifyAcceptTheadExit thread." << std::endl;
    while(!g_acceptThreadExitFlag)
    {
        if(!g_ExitFlag)
            continue;
        std::cout << "g_ExitFlag = true. Begin to notifyAcceptTheadExit." << std::endl;
        int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(conn_fd < 0)
        {
            std::cout << "notifyAcceptTheadExit: Create Socket Failed." << std::endl;
        }
        else
        {
            std::cout << "notifyAcceptTheadExit: Create Socket success." << std::endl;
        }
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof (struct sockaddr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_PORT);
        serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS.c_str());
        
        // send connect request to server.
        std::cout << "notifyAcceptTheadExit: Begin to connect to localhost ..." << std::endl;
        if (connect(conn_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0)
        {
           std::cout << "notifyAcceptTheadExit: Connect Failed." << std::endl;
        }
        else
        {
           std::cout << "notifyAcceptTheadExit: Connect Sucess." << std::endl;
        }
        usleep(200);
    }
    std::cout << "notifyAcceptTheadExit Thead Exit." << std::endl;
}


bool isStop()
{
    usleep(20000);
    std::cout << "Hello, This is isStop thread." << std::endl;
    while(1)
    {
        if(access("stop", F_OK) == 0)
        {
            g_ExitFlag = true;
            break;
        }
    }
    std::cout << "isStop Thread Exit." << std::endl;
}

void sendThread(int index)
{
    usleep(20000);
    std::cout << "Hello, This is send thread, index = " << index << std::endl;
    usleep(200000);
    g_mutex.lock();
    g_bSendThreadIdleFlag[index] = true;
    g_mutex.unlock();
    std::cout << "Receive Thread Exits. index = " << index << std::endl;
}


void recvThread(int index)
{
    usleep(20000);
    std::cout << "Hello, This is receive thread, index = " << index << std::endl;
    usleep(200000);
    g_mutex.lock();
    g_bRecvThreadIdleFlag[index] = true;
    g_mutex.unlock();
    std::cout << "Receive Thread Exits. index = " << index << std::endl;
}


void updateSystemStatus()
{
    usleep(20000);
    std::cout << "Hello, This is updateSystemStatus thread." << std::endl;
    // find some idle socket and close
    while(!g_acceptThreadExitFlag)
    {
        g_mutex.lock();
        for(int i=0; i < MAX_CLIENT_NUM; ++i)
        {
            if(g_bSendThreadIdleFlag[i] && g_bRecvThreadIdleFlag[i] && g_ClientSocketFd[i] != -1)
            {
                close(g_ClientSocketFd[i]);
                g_ClientSocketFd[i] = -1;
                g_ClientSocketAliveStatus[i] = false;
                memset(&g_ClentAddrArray[i], 0, sizeof(struct sockaddr_in));
                g_ptSendThread[i] = nullptr;
                g_ptRecvThread[i] = nullptr;
            }
        }
        g_mutex.unlock();
        usleep(200000);
    }
    std::cout << "UpdateSystemStatus Thread Exits." << std::endl;
}


void acceptThread()
{
    usleep(20000);
    std::cout << "Hello, this is acceptThread." << std::endl;
    struct sockaddr_in t_ClientAddr;
    socklen_t t_ClientLen = sizeof(struct sockaddr_in);
    while(!g_ExitFlag)
    {
        int t_SocketIdleIndex = findIdleSocketIdx();
        if(t_SocketIdleIndex == -1)
            continue;
        memset(&t_ClientAddr, 0, sizeof(struct sockaddr_in));
        std::cout << "acceptThread: Ready to accept a connect request." << std::endl;
        g_AcceptSocket = accept(g_Socket, (struct sockaddr *)&t_ClientAddr, &t_ClientLen);
        if(g_AcceptSocket < 0)
        {
            std::cout << "acceptThread: Accept Failed" << std::endl;
            // break;
            continue;
        }
        else
        {
            std::cout << "acceptThread: Accept a new client, ip: " << inet_ntoa(t_ClientAddr.sin_addr) << std::endl;
        }
        g_mutex.lock();
        g_ClientSocketAliveStatus[t_SocketIdleIndex] = true;
        g_ClientSocketFd[t_SocketIdleIndex] = g_AcceptSocket;
        g_ClentAddrArray[t_SocketIdleIndex] = t_ClientAddr;
        // Create send thread
        g_ptSendThread[t_SocketIdleIndex] = new std::thread(sendThread, t_SocketIdleIndex);
        g_ptSendThread[t_SocketIdleIndex]->detach();
        g_bSendThreadIdleFlag[t_SocketIdleIndex] = false;
        // Create receive thread
        g_ptRecvThread[t_SocketIdleIndex] = new std::thread(recvThread, t_SocketIdleIndex);
        g_ptRecvThread[t_SocketIdleIndex]->detach();
        g_bRecvThreadIdleFlag[t_SocketIdleIndex] = false;
        g_mutex.unlock();
    }
    bool allSocketClosedFlag = false;
    while(allSocketClosedFlag)
    {
        int i;
        for(; i < MAX_CLIENT_NUM; ++i)
        {
            if(g_ClientSocketAliveStatus[i])
                break;
        }
        if(i == MAX_CLIENT_NUM)
            allSocketClosedFlag = true;
    }
    g_acceptThreadExitFlag = true;
    std::cout << "Accept Thread Exits." << std::endl;
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
    std::thread t_updateSystemStatusThread(updateSystemStatus);
    // int ret = pthread_create(&t_AcceptThread, NULL, acceptThread, NULL);
    usleep(2000);
    t_AcceptThread.join();
    t_isStopThread.join();
    t_notifyAcceptTheadExit.join();
    t_updateSystemStatusThread.join();
    usleep(2000);
    close(g_Socket);
    std::cout << "Main Thread Exit." << std::endl;

    return 0;
}

