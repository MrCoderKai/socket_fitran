#ifndef _SOCKET_COMMUNICATION_H_
#define _SOCKET_COMMUNICATION_H_
#include <iostream>
#include <cstring> // memset uses this head file
// extern  constexpr int MAX_SOCKET_PACKAGE_LEN;

constexpr int SOCKET_RECV_UNIT_MAX_LEN = 1024 * 1024 * 20; // 20M
constexpr int MAX_SOCKET_PACKAGE_LEN = 1024;
constexpr int SERVER_PORT = 24647;
constexpr int MAX_CLIENT_NUM = 20;
extern const std::string SERVER_IP_ADDRESS;// = "127.0.0.1";

#ifndef __UCHAR__
#define __UCHAR__
typedef unsigned char UCHAR;
#endif


typedef enum
{
    IDLE,
    CLIENT_SERVER_REQ_DATA,
    SERVER_CLIENT_REQ_DATA,
    CLIENT_SERVER_SEND_DATA,
    SERVER_CLIENT_SEND_DATA
    
} ENUM_MSG_TYPE;


struct STRU_PRIMITIVE_HEAD
{
    UCHAR m_ucMsgType;
    UCHAR m_ucReserve[3];
} ;


struct STRU_MSG_REPORT_DATA
{
    struct STRU_PRIMITIVE_HEAD m_struHeader;
    int m_iNewFileFlag;
    int m_iPacketIdx;
    int m_iLastPacketFlag;
    int m_iValidByteNum;
    int m_iSectionByteNum;
    char m_cData[MAX_SOCKET_PACKAGE_LEN];
    //char m_cData[1024]; // ATTENTION!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    STRU_MSG_REPORT_DATA()
    {
        m_iNewFileFlag = 0;
        m_iPacketIdx = 0;
        m_iLastPacketFlag = 0;
        m_iValidByteNum = 0;
        m_iSectionByteNum = 0;
        memset(m_cData, 0, sizeof(m_cData));
    }
} ;


// Declaration of functions
int sendFile(int socketfd, std::string filename, int mode);
int recvFile(int socketfd);
#endif
