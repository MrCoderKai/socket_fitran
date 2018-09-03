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
#include <ctime>

//const static int MAX_SOCKET_PACKAGE_LEN = 1024;
//const static int MAX_CLIENT_NUM = 20;
//const static int SERVER_PORT = 24645;
//std::string SERVER_IP_ADDRESS = "39.107.75.198";
const std::string SERVER_IP_ADDRESS = "127.0.0.1";

bool g_ClientSocketAliveStatus[MAX_CLIENT_NUM];

struct STRU_SERVER_CONTROL g_struServerControl[MAX_CLIENT_NUM];
// int g_ClientSocketFd[MAX_CLIENT_NUM];
// struct sockaddr_in g_ClentAddrArray[MAX_CLIENT_NUM];
// bool g_bSendThreadIdleFlag[MAX_CLIENT_NUM];
// std::thread* g_ptSendThread[MAX_CLIENT_NUM];
// bool g_bRecvThreadIdleFlag[MAX_CLIENT_NUM];
// std::thread* g_ptRecvThread[MAX_CLIENT_NUM];
// UCHAR* g_ucRecvBuffer[MAX_CLIENT_NUM];
// int g_iReadOffset[g_ucRecvBuffer];
// int g_iWriteOffset[g_ucRecvBuffer];

std::mutex g_mutex;

int g_Socket;
int g_AcceptSocket;
struct sockaddr_in g_ServerSockAddr;
bool g_ExitFlag = false;
bool g_acceptThreadExitFlag = false;

//void initial()
//{
//    memset(&g_ClentAddrArray, 0, sizeof(g_ClentAddrArray));
//    memset(&g_ClientSocketAliveStatus, 0, sizeof(g_ClientSocketAliveStatus));
//    memset(&g_ptSendThread, 0, sizeof(g_ptSendThread));
//    memset(&g_ptRecvThread, 0, sizeof(g_ptRecvThread));
//    memset(&g_iReadOffset, 0, sizeof(g_iReadOffset));
//    memset(&g_iWriteOffset, 0, sizeof(g_iWriteOffset));
//    for(int i=0; i < MAX_CLIENT_NUM; ++i)
//    {
//        g_ClientSocketFd[i] = -1;
//        g_bSendThreadIdleFlag[i] = true;
//        g_bRecvThreadIdleFlag[i] = true;
//        g_ucRecvBuffer[i] = nullptr;
//    }
//}

void initial()
{
    srand((unsigned)time(NULL));
    memset(&g_ClientSocketAliveStatus, 0, sizeof(g_ClientSocketAliveStatus));
    for(int i=0; i < MAX_CLIENT_NUM; ++i)
    {
        g_struServerControl[i].m_ClientSocketFd = -1;
        g_struServerControl[i].m_bSendThreadIdleFlag = true;
        g_struServerControl[i].m_ptSendThread = nullptr;
        g_struServerControl[i].m_bRecvThreadIdleFlag = true;
        g_struServerControl[i].m_ptRecvThread = nullptr;
        g_struServerControl[i].m_iReadOffset = 0;
        g_struServerControl[i].m_iWriteOffset = 0;
        memset(&g_struServerControl[i].m_ClientAddr, 0, sizeof(g_struServerControl[i].m_ClientAddr));
        memset(&g_struServerControl[i].m_ucReceiveBuffer, 0, sizeof(g_struServerControl[i].m_ucReceiveBuffer));
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
        // serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS.c_str());
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        // send connect request to server.
        std::cout << "notifyAcceptTheadExit: Begin to connect to localhost ..." << std::endl;
        if (connect(conn_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0)
        // if (connect(g_Socket, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0)
        {
           std::cout << "notifyAcceptTheadExit: Connect Failed." << std::endl;
           close(conn_fd);
           usleep(200000);
           continue;
        }
        else
        {
           std::cout << "notifyAcceptTheadExit: Connect Sucess." << std::endl;
        }
        close(conn_fd);
        usleep(200000);
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


void assignId(int index)
{
    int t_ClientId = rand();
    struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID t_struAssignId;
    t_struAssignId.m_struHeader.m_ucMsgType = SERVER_CLIENT_ASSIGN_ID;
    t_struAssignId.m_iId = t_ClientId;
    t_struAssignId.m_tRNTI = g_struServerControl[index].m_tRNTI;
    t_struAssignId.m_iId = t_ClientId;std::cout << "assignId: client m_tRNTI = " << g_struServerControl[index].m_tRNTI << ". assignId client id = " << t_ClientId << std::endl;
    send(g_struServerControl[index].m_ClientSocketFd, &t_struAssignId, sizeof(t_struAssignId), 0);
}


void sendThread(int index)
{
    usleep(20000);
    std::cout << "Hello, This is send thread, index = " << index << std::endl;
    // char t_cBuffer[200]; // = {"Hello. this is server."};
    // int t_ClientId = rand();
    // std::string buf = "Hello, This information comes from server. Your Id is " + std::to_string(t_ClientId);
    // strcpy(t_cBuffer, buf.c_str());
    // send(g_struServerControl[index].m_ClientSocketFd, t_cBuffer, buf.size(), 0);

    // Begin to send file
    // sendFile(g_struServerControl[index].m_ClientSocketFd, "screen_shoot_20180902231104.png", 0);
    
    // Send close signal
    // struct STRU_CLOSE_SIGNAL t_struCloseSignal;
    // t_struCloseSignal.m_struHeader.m_ucMsgType = SERVER_CLIENT_CLOSE_SIGNAL;
    // send(g_struServerControl[index].m_ClientSocketFd, &t_struCloseSignal, sizeof(t_struCloseSignal), 0);
    
    usleep(200000);
    g_mutex.lock();
    g_struServerControl[index].m_bSendThreadIdleFlag = true;
    std::cout << "Send Thread Exits. index = " << index << std::endl;
    g_mutex.unlock();
}


int getRNTI(struct STRU_RECV_MANAGER& t_struRecvManager)
{
    int t_RNTI = -1;
    if(t_struRecvManager.m_iReadOffset + sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID) <= SOCKET_RECV_UNIT_MAX_LEN * 2)
    {
        struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID* t_pStruAssignId = (struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID*)(t_struRecvManager.m_ucReceiveBufferAddr + t_struRecvManager.m_iReadOffset);
        t_struRecvManager.m_iReadOffset += sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID);
        t_RNTI = t_pStruAssignId->m_tRNTI;
    }
    else
    {
        UCHAR* t_buffer = new UCHAR[sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID)];
        // copy the first part
        int t_iFirstSize = SOCKET_RECV_UNIT_MAX_LEN * 2 - t_struRecvManager.m_iReadOffset;
        memcpy(t_buffer, t_struRecvManager.m_ucReceiveBufferAddr + t_struRecvManager.m_iReadOffset, t_iFirstSize);
        // copy the second part
        int t_iSecondSize = sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID) - t_iFirstSize;
        memcpy(t_buffer + t_iFirstSize, t_struRecvManager.m_ucReceiveBufferAddr, t_iSecondSize);
        struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID* t_pStruAssignId = (struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID*)t_buffer;
        t_RNTI = t_pStruAssignId->m_tRNTI;
        delete []t_buffer;
        t_struRecvManager.m_iReadOffset = t_iSecondSize;
    }
    t_struRecvManager.m_iReadOffset %= (SOCKET_RECV_UNIT_MAX_LEN * 2);
    return t_RNTI;
}

void recvThread(int index)
{
    usleep(20000);
    std::cout << "Hello, This is receive thread, index = " << index << std::endl;
    g_struServerControl[index].m_iReadOffset = 0;
    g_struServerControl[index].m_iWriteOffset = 0;
    memset(&g_struServerControl[index].m_ucReceiveBuffer, 0, sizeof(g_struServerControl[index].m_ucReceiveBuffer));
    
    struct STRU_RECV_MANAGER t_struRecvManager(g_struServerControl[index].m_iReadOffset, g_struServerControl[index].m_iWriteOffset); // Remeber to initialize reference variables in struct
    t_struRecvManager.m_ucReceiveBufferAddr = g_struServerControl[index].m_ucReceiveBuffer;

    int cnt = 0;
    bool t_bClientCloseFlag = false;
    while(1)
    {
        int t_iEmptySize = 0;
        if(t_struRecvManager.m_iWriteOffset >= t_struRecvManager.m_iReadOffset)
            t_iEmptySize = SOCKET_RECV_UNIT_MAX_LEN * 2 - (t_struRecvManager.m_iWriteOffset - t_struRecvManager.m_iReadOffset);
        else
            t_iEmptySize = t_struRecvManager.m_iReadOffset - t_struRecvManager.m_iWriteOffset;
        cnt = (int)recv(g_struServerControl[index].m_ClientSocketFd, t_struRecvManager.m_ucReceiveBufferAddr + t_struRecvManager.m_iWriteOffset, t_iEmptySize, 0);
        std::cout << "Client Index = " << index << ". After receive. Receive Bytes cnt = " << cnt << std::endl;
        if(cnt == -1)
        {
            std::cout << "recvThread: Receivd Failed." << std::endl;
            break;
        }
        t_struRecvManager.m_iWriteOffset += cnt;
        if(t_struRecvManager.m_iWriteOffset >= SOCKET_RECV_UNIT_MAX_LEN * 2)
            t_struRecvManager.m_iWriteOffset = 0;
        bool t_bIsLoopFlag = true;
        {
            UCHAR t_ucPrimitiveType = *(UCHAR*)(t_struRecvManager.m_ucReceiveBufferAddr + t_struRecvManager.m_iReadOffset);
            struct STRU_MSG_REPORT_DATA* t_pStruMsgReportData;
            int t_iValidSize = 0;
            if(t_struRecvManager.m_iWriteOffset >= t_struRecvManager.m_iReadOffset)
                t_iValidSize = t_struRecvManager.m_iWriteOffset - t_struRecvManager.m_iReadOffset;
            else
                t_iValidSize = SOCKET_RECV_UNIT_MAX_LEN * 2 - (t_struRecvManager.m_iReadOffset - t_struRecvManager.m_iWriteOffset);
            switch(t_ucPrimitiveType)
            {
                case CLIENT_SERVER_ACCESS_REQ:
                    std::cout << "t_ucPrimitiveType = CLIENT_SERVER_ACCESS_REQ" << std::endl;
                    if(t_iValidSize >= sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID))
                    {
                        g_struServerControl[index].m_tRNTI = getRNTI(t_struRecvManager);
                        assignId(index);
                    }
                    else
                    {
                        t_bIsLoopFlag = false;
                    }
                    break;
                case CLIENT_SERVER_REQ_DATA:
                    std::cout << "t_ucPrimitiveType = CLIENT_SERVER_REQ_DATA" << std::endl;
                    if(t_iValidSize >= sizeof(struct STRU_REQ_DATA))
                    {
                        std::string t_sFileName = getReqFilename(t_struRecvManager);
                        // Begin to send file
                        // sendFile(g_struServerControl[index].m_ClientSocketFd, "screen_shoot_20180902231104.png", 0);
                        std::cout << "Client Request Filename = " << t_sFileName << std::endl;
                        sendFile(g_struServerControl[index].m_ClientSocketFd, t_sFileName, 0);
                        std::cout << "recvThread: send file done." << std::endl;
                    }
                    else
                    {
                        t_bIsLoopFlag = false;
                    }
                case CLIENT_SERVER_CLOSE_SIGNAL:
                    std::cout << "t_ucPrimitiveType = CLIENT_SERVER_CLOSE_SIGNAL" << std::endl;
                    sendServerClientCloseSignal(g_struServerControl[index].m_ClientSocketFd);
                    t_bClientCloseFlag = true;
                    break;
                default:
                    break;
            }
        }
        if(t_bClientCloseFlag)
            break;
    }


    usleep(200000);
    g_mutex.lock();
    g_struServerControl[index].m_bRecvThreadIdleFlag = true;
    std::cout << "Receive Thread Exits. index = " << index << std::endl;
    g_mutex.unlock();
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
            if(g_struServerControl[i].m_bSendThreadIdleFlag && g_struServerControl[i].m_bRecvThreadIdleFlag && g_struServerControl[i].m_ClientSocketFd != -1)
            {
                close(g_struServerControl[i].m_ClientSocketFd);
                g_struServerControl[i].m_ClientSocketFd = -1;
                g_ClientSocketAliveStatus[i] = false;
                memset(&g_struServerControl[i].m_ClientAddr, 0, sizeof(g_struServerControl[i].m_ClientAddr));
                g_struServerControl[i].m_ptSendThread = nullptr;
                g_struServerControl[i].m_ptRecvThread = nullptr;
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
    socklen_t t_ClientLen = sizeof(t_ClientAddr);
    while(!g_ExitFlag)
    {
        int t_SocketIdleIndex = findIdleSocketIdx();
        if(t_SocketIdleIndex == -1)
            continue;
        memset(&t_ClientAddr, 0, sizeof(struct sockaddr_in));
        std::cout << "acceptThread: Ready to accept a connect request." << std::endl;
        g_AcceptSocket = accept(g_Socket, (struct sockaddr *)&t_ClientAddr, &t_ClientLen);
        if(g_ExitFlag) // This code is very important. Otherwise, the while loop will excute forever, because no client send CLIENT_SERVER_CLOSE_SIGNAL.
        {
            if(g_AcceptSocket >= 0)
                close(g_AcceptSocket);
            break;
        }
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
        g_struServerControl[t_SocketIdleIndex].m_ClientSocketFd = g_AcceptSocket;
        g_struServerControl[t_SocketIdleIndex].m_ClientAddr = t_ClientAddr;
        // Create send thread
        g_struServerControl[t_SocketIdleIndex].m_ptSendThread = new std::thread(sendThread, t_SocketIdleIndex);
        g_struServerControl[t_SocketIdleIndex].m_ptSendThread->detach();
        g_struServerControl[t_SocketIdleIndex].m_bSendThreadIdleFlag = false;
        // Create receive thread
        g_struServerControl[t_SocketIdleIndex].m_ptRecvThread = new std::thread(recvThread, t_SocketIdleIndex);
        g_struServerControl[t_SocketIdleIndex].m_ptRecvThread->detach();
        g_struServerControl[t_SocketIdleIndex].m_bRecvThreadIdleFlag = false;
        g_mutex.unlock();
    }
    bool allSocketClosedFlag = false;
    while(!allSocketClosedFlag)
    {
        g_mutex.lock();
        std::cout << "acceptThread: Checking whether all send & recv thread exits." << std::endl;
        int i = 0;
        for(; i < MAX_CLIENT_NUM; ++i)
        {
            if(g_ClientSocketAliveStatus[i])
            {
                std::cout << "acceptThread: index = " << i << " is still running." << std::endl;
                break;
            }
        }
        if(i == MAX_CLIENT_NUM)
            allSocketClosedFlag = true;
        std::cout << "acceptThread: i = " << i << "\t allSocketClosedFlag = " << allSocketClosedFlag << std::endl;
        g_mutex.unlock();
        std::cout << "acceptThread: Check finished." << std::endl;
        usleep(20000);
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
    std::cout << "Hello, this is Server socket programming demo." << std::endl;
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

