#ifndef _SOCKET_COMMUNICATION_H_
#define _SOCKET_COMMUNICATION_H_
#include <iostream>
#include <cstring> // memset uses this head file
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <mutex>
#include <thread>
#include <fstream>
#include <string>
// extern  constexpr int MAX_SOCKET_PACKAGE_LEN;

constexpr int SOCKET_RECV_UNIT_MAX_LEN = 1024 * 1 * 20; // 20M
constexpr int MAX_SOCKET_PACKAGE_LEN = 1024;
constexpr int SERVER_PORT = 24650;
constexpr int MAX_CLIENT_NUM = 20;
extern const std::string SERVER_IP_ADDRESS;// = "127.0.0.1";

#ifndef __UCHAR__
#define __UCHAR__
typedef unsigned char UCHAR;
#endif


typedef enum
{
    CLIENT_SERVER_ACCESS_REQ = 10,
    SERVER_CLIENT_ASSIGN_ID,
    CLIENT_SERVER_REQ_DATA,
    SERVER_CLIENT_REQ_DATA,
    CLIENT_SERVER_SEND_DATA,
    SERVER_CLIENT_SEND_DATA,
    CLIENT_SERVER_CLOSE_SIGNAL,
    SERVER_CLIENT_CLOSE_SIGNAL
} ENUM_MSG_TYPE;


struct STRU_PRIMITIVE_HEAD
{
    UCHAR m_ucMsgType;
    UCHAR m_ucReserve[3];
} ;


struct STRU_CLOSE_SIGNAL
{
    struct STRU_PRIMITIVE_HEAD m_struHeader;
};

struct STRU_MSG_REPORT_DATA
{
    struct STRU_PRIMITIVE_HEAD m_struHeader;
    int m_iId;
    int m_iNewFileFlag;
    int m_iPacketIdx;
    int m_iLastPacketFlag;
    int m_iValidByteNum;
    int m_iSectionByteNum;
    char m_cData[MAX_SOCKET_PACKAGE_LEN];
    //char m_cData[1024]; // ATTENTION!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    STRU_MSG_REPORT_DATA()
    {
        m_iId = 0;
        m_iNewFileFlag = 0;
        m_iPacketIdx = 0;
        m_iLastPacketFlag = 0;
        m_iValidByteNum = 0;
        m_iSectionByteNum = 0;
        memset(m_cData, 0, sizeof(m_cData));
    }
} ;

struct STRU_MSG_SERVER_CLIENT_ASSIGN_ID
{
    struct STRU_PRIMITIVE_HEAD m_struHeader;
    int m_tRNTI; // This id is used in access stage.
    int m_iId;   // This id is assigned by server. After access, it is used as the id of client in communication.
};

struct STRU_SERVER_CONTROL
{
    int m_ClientSocketFd;
    // bool m_ClientSocketAliveStatus;
    struct sockaddr_in m_ClientAddr;
    int m_tRNTI;
    int m_iClientId;
    bool m_bSendThreadIdleFlag;
    std::thread* m_ptSendThread;
    bool m_bRecvThreadIdleFlag;
    std::thread* m_ptRecvThread;
    int m_iReadOffset;
    int m_iWriteOffset;
    std::mutex m_ReadMutex;
    std::mutex m_WriteMutex;
    UCHAR m_ucReceiveBuffer[SOCKET_RECV_UNIT_MAX_LEN * 2];
    STRU_SERVER_CONTROL()
    {
        m_ClientSocketFd = -1;
        // m_ClientSocketAliveStatus = false;
        memset(&m_ClientAddr, 0, sizeof(m_ClientAddr));
        m_tRNTI = -1;
        m_iClientId = -1;
        m_bSendThreadIdleFlag = true;
        m_ptSendThread = nullptr;
        m_bRecvThreadIdleFlag = true;
        m_ptRecvThread = nullptr;
        m_iReadOffset = 0;
        m_iWriteOffset = 0;
        memset(&m_ucReceiveBuffer, 0, sizeof(m_ucReceiveBuffer));
    }
} ;


struct STRU_REQ_DATA
{
    struct STRU_PRIMITIVE_HEAD m_struHeader;
    int m_iFileNameLen;
    char m_ucFileName[1024];
    STRU_REQ_DATA()
    {
        m_iFileNameLen = 0;
        memset(&m_ucFileName, 0, sizeof(m_ucFileName));
    }
} ;


struct STRU_RECV_MANAGER
{
    int m_iUserID;
    int m_iRecvStatus; // 0: not receive 1:receiving
    int m_iPacketCurrentIndex;
    int m_iPacketReceiveSize;
    int m_iBufferOffset;
    int& m_iReadOffset;
    int& m_iWriteOffset;
    UCHAR* m_ucReceiveBufferAddr;
    long int m_i64FileSize;
    std::fstream m_WriteFileStream;
    std::string m_sFileName;
    STRU_RECV_MANAGER(int& readOffset, int& writeOffset): m_iReadOffset(readOffset), m_iWriteOffset(writeOffset)
    {
        m_iUserID = 0;
        m_iRecvStatus = 0;
        m_iPacketCurrentIndex = 0;
        m_iPacketReceiveSize = 0;
        m_iBufferOffset = 0;
        m_ucReceiveBufferAddr = nullptr;
        m_i64FileSize = 0;
        m_sFileName = "recv_0000.txt";
    }
} ;

// Declaration of functions
int sendFile(int socketfd, std::string filename, int mode);
int recvFile(int socketfd);
int processData(struct STRU_RECV_MANAGER& t_struRecvManager);
void sendServerClientCloseSignal(int socketfd);
void sendClientServerCloseSignal(int socketfd);
void sendReqDataSignal(int socketfd, int mode, std::string filename);
std::string getReqFilename(struct STRU_RECV_MANAGER& t_strRecvManager);
#endif
