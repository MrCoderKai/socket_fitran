#ifndef _SOCKET_COMMUNICATION_H_
#define _SOCKET_COMMUNICATION_H_

// extern  constexpr int MAX_SOCKET_PACKAGE_LEN;

typedef unsigned char UCHAR;

typedef enum
{
    CLIENT_SERVER_REQ_DATA,
    SERVER_CLIENT_REQ_DATA,
    CLIENT_SERVER_SEND_DATA,
    SERVER_CLIENT_SEND_DATA
    
} ENUM_MSG_TYPE;


typedef struct
{
    UCHAR m_ucMsgType;
    UCHAR m_ucReserve[3];
} STRU_PRIMITIVE_HEAD;


typedef struct
{
    STRU_PRIMITIVE_HEAD m_struHeader;
    char m_csFileName[20];  // file name
    int m_iNewFileFlag;
    int m_iPacketIdx;
    int m_iLastPacketFlag;
    int m_iValidByteNum;
    int m_iSectionByteNum;
    //char m_cData[MAX_SOCKET_PACKAGE_LEN];
    char m_cData[1024]; // ATTENTION!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
} STRU_MSG_REPORT_DATA;


#endif
