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
int g_iClientId = -1;
int g_tRNTI = -1;
int g_iAccessFailedNum = 0;
bool g_bAccessFlag = false;
// std::string g_sReqFileName = "data.txt";
// std::string g_sReqFileName = "screen_shoot_20180902231104.png";
std::string g_sReqFileName = "paper.pdf";

struct sockaddr_in g_ServerSockAddr;
UCHAR g_ucReceiveBuffer[SOCKET_RECV_UNIT_MAX_LEN * 2];
int g_iWriteOffset = 0;
int g_iReadOffset = 0;
void initial()
{
    srand((unsigned)time(NULL));
}


void accessOnce()
{
    if(g_tRNTI == -1)
        g_tRNTI = rand();
    std::cout << "accessOnce: my m_tRNTI = " << g_tRNTI << std::endl;
    std::cout << "accessOnce: my g_iClientId = " << g_iClientId << std::endl;
    struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID t_struMsgAssignId;
    t_struMsgAssignId.m_struHeader.m_ucMsgType = CLIENT_SERVER_ACCESS_REQ;
    t_struMsgAssignId.m_tRNTI = g_tRNTI;
    send(g_Socket, &t_struMsgAssignId, sizeof(t_struMsgAssignId), 0);
    std::cout << "accessOnce: access once finished." << std::endl;
}



void sendThread()
{
    usleep(20000);
    std::cout << "Hello, This is send thread" << std::endl;
    while(!g_bAccessFlag)
    {
        accessOnce();
        sleep(2);
    }

    // Send Request Data
    sendReqDataSignal(g_Socket, 1, g_sReqFileName);

    // Send close signal
    sendClientServerCloseSignal(g_Socket);
    // struct STRU_CLOSE_SIGNAL t_struCloseSignal;
    // t_struCloseSignal.m_struHeader.m_ucMsgType = CLIENT_SERVER_CLOSE_SIGNAL;
    // send(g_Socket, &t_struCloseSignal, sizeof(t_struCloseSignal), 0);

    usleep(2000000);
    g_mutex.lock();
    std::cout << "Send Thread Exits." << std::endl;
    g_mutex.unlock();
}


int getId()
{
    int id = -1;
    if(g_iReadOffset + sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID) <= SOCKET_RECV_UNIT_MAX_LEN * 2)
    {
        struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID* t_pStruAssignId = (struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID*)(g_ucReceiveBuffer + g_iReadOffset);
        g_iReadOffset += sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID);
        std::cout << "getId: received m_tRNTI = " << t_pStruAssignId->m_tRNTI << std::endl;
        if(t_pStruAssignId->m_tRNTI != g_tRNTI)
        {
            g_iAccessFailedNum += 1;
        }
        else
        {
            id = t_pStruAssignId->m_iId;
            g_iAccessFailedNum = 0;
            g_bAccessFlag = true;
        }
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
        g_iReadOffset = t_iSecondSize;
        std::cout << "getId: received m_tRNTI = " << t_pStruAssignId->m_tRNTI << std::endl;
        if(t_pStruAssignId->m_tRNTI != g_tRNTI)
        {
            g_iAccessFailedNum += 1;
        }
        else
        {
            id = t_pStruAssignId->m_iId;
            g_iAccessFailedNum = 0;
            g_bAccessFlag = true;
        }
        delete []t_buffer;
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
    int t_iCntPart1 = 0;
    int t_iCntPart2 = 0;
    while(1)
    {
        int t_iEmptySizePart1 = 0;
        int t_iEmptySizePart2 = 0;
        t_iCntPart1 = 0;
        t_iCntPart2 = 0;
        if(g_iWriteOffset >= g_iReadOffset)
        {
            // t_iEmptySize = SOCKET_RECV_UNIT_MAX_LEN * 2 - (g_iWriteOffset - g_iReadOffset);
            t_iEmptySizePart1 = SOCKET_RECV_UNIT_MAX_LEN * 2 - g_iWriteOffset;
            t_iEmptySizePart2 = (int)(g_iReadOffset * 0.8);
        }
        else if(g_iWriteOffset < g_iReadOffset)
        {
            // t_iEmptySize = g_iReadOffset - g_iWriteOffset;
            t_iEmptySizePart1 = (int)((g_iReadOffset - g_iWriteOffset) * 0.8);
            t_iEmptySizePart2 = 0;
        }
        std::cout << "*t_iEmptySizePart1 = " << t_iEmptySizePart1 << std::endl;
        std::cout << "*t_iEmptySizePart2 = " << t_iEmptySizePart2 << std::endl;
        // t_iCntPart1 = (int)recv(g_Socket, g_ucReceiveBuffer + g_iWriteOffset, t_iEmptySize, 0);
        t_iCntPart1 = (int)recv(g_Socket, g_ucReceiveBuffer + g_iWriteOffset, t_iEmptySizePart1, 0);
        std::cout << "*After receive. Receive Bytes t_iCntPart1 = " << t_iCntPart1 << std::endl;
        if(t_iCntPart1 == -1)
        {
            std::cout << "recvThread: Receivd Failed." << std::endl;
            break;
        }
        // the ratio 0.3 here is to avoid g_iWriteOffset == g_iReadOffset, otherwise, it will be hard to handle in the following, because I can know whether the whole buffer is empty or full with data.
        if(t_iEmptySizePart2 >= (int)(SOCKET_RECV_UNIT_MAX_LEN * 2 * 0.3) && t_iCntPart1 == t_iEmptySizePart1) // receive if and only if there are data to receive and there is enough free memeory.
        // if(t_iEmptySizePart2 != 0)
        {
            t_iCntPart2 = (int)recv(g_Socket, g_ucReceiveBuffer, t_iEmptySizePart2, 0);
            if(t_iCntPart2 == -1)
            {
                std::cout << "recvThread: Receivd Failed 2." << std::endl;
                break;
            }
            std::cout << "*After receive. Receive Bytes t_iCntPart2 = " << t_iCntPart2 << std::endl;
        }
        else
        {
            t_iCntPart2 = 0;
        }
        g_iWriteOffset = (g_iWriteOffset + t_iCntPart1 + t_iCntPart2) % (SOCKET_RECV_UNIT_MAX_LEN * 2);
        // if(g_iWriteOffset >= SOCKET_RECV_UNIT_MAX_LEN * 2)
        //     g_iWriteOffset = 0;
        std::cout << "*g_iWriteOffset = " << g_iWriteOffset << std::endl;
        std::cout << "*g_iReadOffset = " << g_iReadOffset << std::endl;
        std::cout << "*receive buffer size = " << SOCKET_RECV_UNIT_MAX_LEN * 2 << std::endl;
        int t_iValidSize = 0;
        if(g_iWriteOffset >= g_iReadOffset)
            t_iValidSize = g_iWriteOffset - g_iReadOffset;
        else
            t_iValidSize = SOCKET_RECV_UNIT_MAX_LEN * 2 - (g_iReadOffset - g_iWriteOffset);
        bool t_bIsLoopFlag = true;
        // while(g_iWriteOffset >= g_iReadOffset && t_bIsLoopFlag)
        while(t_iValidSize > 0 && t_bIsLoopFlag)
        {
            // Get Msg Primitive
            UCHAR t_ucPrimitiveType = *(UCHAR*)(g_ucReceiveBuffer + g_iReadOffset);
            struct STRU_MSG_REPORT_DATA* t_pStruMsgReportData;
            if(g_iWriteOffset >= g_iReadOffset)
                t_iValidSize = g_iWriteOffset - g_iReadOffset;
            else
                t_iValidSize = SOCKET_RECV_UNIT_MAX_LEN * 2 - (g_iReadOffset - g_iWriteOffset);
            std::cout << "t_ucPrimitiveType = " << int(t_ucPrimitiveType) << std::endl; 
            std::cout << "g_iWriteOffset = " << g_iWriteOffset << std::endl;
            std::cout << "g_iReadOffset = " << g_iReadOffset << std::endl;
            std::cout << "t_iValidSize = " << t_iValidSize << std::endl;
            if(t_iValidSize == 0)
                break;
            switch(t_ucPrimitiveType)
            {
                case SERVER_CLIENT_ASSIGN_ID:
                    std::cout << "t_ucPrimitiveType = SERVER_CLIENT_ASSIGN_ID" << std::endl;
                    if(t_iValidSize >= sizeof(struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID))
                    {
                        g_iClientId = getId();
                        if(g_iClientId != -1)
                        {
                            t_struRecvManager.m_iUserID = g_iClientId;
                            std::cout << "Assigned ID = " << g_iClientId << std::endl;
                        }
                    }
                    else
                    {
                        t_bIsLoopFlag = false;
                    }
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
                    else
                    {
                        std::cout << "Not enough data. Continue to receive." << std::endl;
                        t_bIsLoopFlag = false;
                    }
                    break;
                case SERVER_CLIENT_CLOSE_SIGNAL:
                    std::cout << "t_ucPrimitiveType = SERVER_CLIENT_CLOSE_SIGNAL" << std::endl;
                    if(t_iValidSize >= sizeof(struct STRU_CLOSE_SIGNAL))
                    {
                        t_bServerCloseFlag = true;
                        g_iReadOffset = (g_iReadOffset + sizeof(struct STRU_CLOSE_SIGNAL)) % (SOCKET_RECV_UNIT_MAX_LEN * 2);
                    }
                    else
                    {
                        t_bIsLoopFlag = false;
                    }
                    break;
                default:
                    std::cout << "t_ucPrimitiveType = default" << std::endl;
                    t_bIsLoopFlag = false;
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

