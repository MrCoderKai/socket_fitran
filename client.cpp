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
const std::string SERVER_IP_ADDRESS = "127.0.0.1";
// std::string SERVER_IP_ADDRESS = "39.107.75.198";
//std::string SERVER_IP_ADDRESS = "127.0.0.1";
int g_Socket;
int g_AcceptSocket;
struct sockaddr_in g_ServerSockAddr;
int g_iClientId = 0;
UCHAR g_ucReceiveBuffer[SOCKET_RECV_UNIT_MAX_LEN * 2];
int g_iWriteOffset = 0;
int g_iReadOffset = 0;
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


int getId()
{
    int id = 0;
    if(g_iReadOffset + sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID) <= SOCKET_RECV_UNIT_MAX_LEN * 2)
    {
        struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID* t_pStruAssignId = (struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID*)(g_ucReceiveBuffer + g_iReadOffset);
        g_iReadOffset += sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID);
        id = t_pStruAssignId->m_iId;
    }
    else
    {
        UCHAR* t_buffer = new UCHAR[sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID)];
        // copy the first part
        int t_iFirstSize = SOCKET_RECV_UNIT_MAX_LEN * 2 - g_iReadOffset;
        memcpy(t_buffer, g_ucReceiveBuffer + g_iReadOffset, t_iFirstSize);
        // copy the second part
        int t_iSecondSize = sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID) - t_iFirstSize;
        memcpy(t_buffer + t_iFirstSize, g_ucReceiveBuffer, t_iSecondSize);
        struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID* t_pStruAssignId = (struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID*)t_buffer;
        id = t_pStruAssignId->m_iId;
        delete []t_buffer;
        g_iReadOffset = t_iSecondSize;
    }
    g_iReadOffset %= (SOCKET_RECV_UNIT_MAX_LEN * 2); 
    return id;
}

void recvThread()
{
    usleep(20000);
    g_iWriteOffset = 0;
    g_iReadOffset = 0;
    std::cout << "Hello, This is receive thread" << std::endl;
    char t_recvBuf[1024];
    bool t_bServerCloseFlag = false;
    struct STRU_RECV_MANAGER t_struRecvManager(g_iReadOffset, g_iWriteOffset); // Remeber to initialize reference variables in struct
    t_struRecvManager.m_ucReceiveBufferAddr = g_ucReceiveBuffer;

    std::cout << "Begin to receive ..." << std::endl;
    int cnt = 0;
    while(1)
    {
        int t_iEmptySize = 0;
        if(g_iWriteOffset >= g_iReadOffset)
            t_iEmptySize = SOCKET_RECV_UNIT_MAX_LEN * 2 - (g_iWriteOffset - g_iReadOffset);
        else if(g_iWriteOffset < g_iReadOffset)
            t_iEmptySize = g_iReadOffset - g_iWriteOffset;
        cnt = (int)recv(g_Socket, g_ucReceiveBuffer + g_iWriteOffset, t_iEmptySize, 0);
        std::cout << "After receive. Receive Bytes cnt = " << cnt << std::endl;
        if(cnt == -1)
        {
            std::cout << "recvThread: Receivd Failed." << std::endl;
            break;
        }
        g_iWriteOffset += cnt;
        if(g_iWriteOffset >= SOCKET_RECV_UNIT_MAX_LEN * 2)
            g_iWriteOffset = 0;
        bool t_bIsLoopFlag = true;
        // while(t_bIsLoopFlag)
        {
            // Get Msg Primitive
            UCHAR t_ucPrimitiveType = *(UCHAR*)(g_ucReceiveBuffer + g_iReadOffset);
            struct STRU_MSG_REPORT_DATA* t_pStruMsgReportData;
            int t_iValidSize = 0;
            if(g_iWriteOffset >= g_iReadOffset)
                t_iValidSize = g_iWriteOffset - g_iReadOffset;
            else
                t_iValidSize = SOCKET_RECV_UNIT_MAX_LEN * 2 - (g_iReadOffset - g_iWriteOffset);
            std::cout << "t_ucPrimitiveType = " << int(t_ucPrimitiveType) << std::endl; 
            std::cout << "g_iWriteOffset = " << g_iWriteOffset << std::endl;
            std::cout << "g_iReadOffset = " << g_iReadOffset << std::endl;
            std::cout << "t_iValidSize = " << t_iValidSize << std::endl;
            switch(t_ucPrimitiveType)
            {
                case SERVER_CLIENT_ASSIGN_ID:
                    std::cout << "t_ucPrimitiveType = SERVER_CLIENT_ASSIGN_ID" << std::endl;
                    if(t_iValidSize >= sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID))
                    {
                        g_iClientId = getId();
                        t_bIsLoopFlag = false;
                        t_struRecvManager.m_iUserID = g_iClientId;
                        std::cout << "Assigned ID = " << g_iClientId << std::endl;
                    }
                    else
                    {
                        t_bIsLoopFlag = false;
                    }
                    std::cout << "OK" << std::endl;
                    break;
                case SERVER_CLIENT_REQ_DATA:
                    std::cout << "t_ucPrimitiveType = SERVER_CLIENT_REQ_DATA" << std::endl;
                    break;
                case SERVER_CLIENT_SEND_DATA:
                    std::cout << "t_ucPrimitiveType = SERVER_CLIENT_SEND_DATA" << std::endl;
                    if(t_iValidSize >= sizeof(struct STRU_MSG_REPORT_DATA))
                    {
                        processData(t_struRecvManager);
                    }
                    break;
                case SERVER_CLIENT_CLOSE_SIGNAL:
                    std::cout << "t_ucPrimitiveType = SERVER_CLIENT_CLOSE_SIGNAL" << std::endl;
                    t_bServerCloseFlag = true;
                    break;
                default:
                    std::cout << "t_ucPrimitiveType = default" << std::endl;
                    break;
            }
            // ATTENTION !!!!!!!!!!!!!!!!!!!!!!!!
            // t_bIsLoopFlag = false;
        }
        if(t_bServerCloseFlag == true)
            break;
    }
    // recvFile(g_Socket);
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

