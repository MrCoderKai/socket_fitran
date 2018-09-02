#include <iostream>
#include "common.h"
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <fstream>
// #include <time.h>
#include <ctime>
#include <string>





std::string datetimeToString(time_t time)
{
    int misc = time % 10000;
    tm *tm_ = localtime(&time);                // 将time_t格式转换为tm结构体
    int year, month, day, hour, minute, second;// 定义时间的各个int临时变量。
    year = tm_->tm_year + 1900;                // 临时变量，年，由于tm结构体存储的是从1900年开始的时间，所以临时变量int为tm_year加上1900。
    month = tm_->tm_mon + 1;                   // 临时变量，月，由于tm结构体的月份存储范围为0-11，所以临时变量int为tm_mon加上1。
    day = tm_->tm_mday;                        // 临时变量，日。
    hour = tm_->tm_hour;                       // 临时变量，时。
    minute = tm_->tm_min;                      // 临时变量，分。
    second = tm_->tm_sec;                      // 临时变量，秒。
    char yearStr[5], monthStr[3], dayStr[3], hourStr[3], minuteStr[3], secondStr[3], miscStr[4];// 定义时间的各个char*变量。
    sprintf(yearStr, "%04d", year);              // 年。
    sprintf(monthStr, "%02d", month);            // 月。
    sprintf(dayStr, "%02d", day);                // 日。
    sprintf(hourStr, "%02d", hour);              // 时。
    sprintf(minuteStr, "%02d", minute);          // 分。
    sprintf(secondStr, "%02d", second);          // 秒。
    sprintf(miscStr, "%04d", misc);              // misc
    char s[20];                                // 定义总日期时间char*变量。
    sprintf(s, "%s%s%s%s%s%s_%s", yearStr, monthStr, dayStr, hourStr, minuteStr, secondStr, miscStr);// 将年月日时分秒合并。
    std::string str(s);                             // 定义string变量，并将总日期时间char*变量作为构造函数的参数传入。
    return str;                                // 返回转换日期时间后的string变量。
}


/* long int GetFileSize(std::string filename)
 * Function: Get file size specified by filename
 * @param: std::string filename      file name
 *
 * return:
 *        -1                         error occurs
 *        >=0                        the size of the specified file
 */
long int GetFileSize(std::string filename)
{
    long int t_liFileSize;
    std::fstream t_fsReadFileStream;
    t_fsReadFileStream.open(filename, std::ios::in|std::ios::binary);
    if(!t_fsReadFileStream.is_open())
    {
        std::cout << "Open file " << filename << "Failed." << std::endl;
        return -1;
    }
    t_fsReadFileStream.seekg(0, std::ios::end);
    t_liFileSize = t_fsReadFileStream.tellg();
    t_fsReadFileStream.close();
    return t_liFileSize;
}


/* sendFile(int socketfd, std::string filename, int mode)
 * Function: Send a file specified by filename to server or client.
 * @param: int socketfd                socket file descriptor
 * @param: std::string filename        the name of file to be sent
 * @param: mode                        0: server    1: client
 * 
 * return:
 *        -1                           error occurs
 *         0                           success
 */
int sendFile(int socketfd, std::string filename, int mode)
{
   // char t_cBuffer[1024];
   // int t_iBufferSize = sizeof(t_cBuffer);
   int t_iReadByteNum = 0;
   int t_iCurrentPacketIndex = 0;
   int t_iTotalPacketNum = 0;
   STRU_MSG_REPORT_DATA t_struMsgReportData;
   auto t_ilFileSize = GetFileSize(filename);
   if(t_ilFileSize == -1)
   {
       std::cout << "sendFile: Error in GetFileSize." << std::endl;
       return -1;
   }
   auto t_ilFileSizeRemain = t_ilFileSize;
   t_iTotalPacketNum = t_ilFileSize / MAX_SOCKET_PACKAGE_LEN;
   if(t_ilFileSize % MAX_SOCKET_PACKAGE_LEN > 0)
       ++t_iTotalPacketNum;
   // std::string t_sInfo = "The size of " + filename + "is " + std::to_string(t_ilFileSize);
   // strcpy(t_cBuffer, t_sInfo.c_str());
   // send(socketfd, t_cBuffer, t_sInfo.size(), 0);
   std::fstream t_fsReadFileStream;
   t_fsReadFileStream.open(filename, std::ios::in|std::ios::binary);
   if(!t_fsReadFileStream.is_open())
   {
       std::cout << "Open file " << filename << "Failed." << std::endl;
       return -1;
   }
   while(t_ilFileSizeRemain > 0)
   {
       if(t_ilFileSizeRemain >= MAX_SOCKET_PACKAGE_LEN)
           t_iReadByteNum = MAX_SOCKET_PACKAGE_LEN;
       else
           t_iReadByteNum = t_ilFileSizeRemain;
       memset(t_struMsgReportData.m_cData, 0, MAX_SOCKET_PACKAGE_LEN);
       t_fsReadFileStream.read(t_struMsgReportData.m_cData, t_iReadByteNum);
       if(t_ilFileSizeRemain == t_ilFileSize)
           t_struMsgReportData.m_iNewFileFlag = 1;
       else
           t_struMsgReportData.m_iNewFileFlag = 0;
       if(mode == 0)
           t_struMsgReportData.m_struHeader.m_ucMsgType = SERVER_CLIENT_SEND_DATA;
       else
           t_struMsgReportData.m_struHeader.m_ucMsgType = CLIENT_SERVER_SEND_DATA;
       t_struMsgReportData.m_iPacketIdx = t_iCurrentPacketIndex;
       ++t_iCurrentPacketIndex;
       if(t_iCurrentPacketIndex == t_iTotalPacketNum)
           t_struMsgReportData.m_iLastPacketFlag = 1;
       else
           t_struMsgReportData.m_iLastPacketFlag = 0;
       t_struMsgReportData.m_iValidByteNum = t_iReadByteNum;
       t_struMsgReportData.m_iSectionByteNum = sizeof(struct STRU_MSG_REPORT_DATA) - MAX_SOCKET_PACKAGE_LEN + t_iReadByteNum;
       send(socketfd, &t_struMsgReportData, t_struMsgReportData.m_iSectionByteNum, 0);
       t_ilFileSizeRemain -= t_iReadByteNum;
   }
   t_fsReadFileStream.close();
   return 0;
}

/* int recvFile(int socketfd)
 * Function Receive a file from server or client
 * @param: int socketfd             socket file descriptor
 *
 * return:
 *        -1                        error occurs
 *         0                        success
 */
int recvFile(int socketfd, int* t_pReadOffset, int* t_pWriteOffset)
{
    UCHAR t_ucRecvBuffer[SOCKET_RECV_UNIT_MAX_LEN * 2];
    bool t_bNewFileFlag = false;
    bool t_bRecvFinishFlag = false;
    time_t t_CurrentTime;
    time(&t_CurrentTime);
    std::string t_sCurrentTime = datetimeToString(t_CurrentTime);
    while(!t_bRecvFinishFlag)
    {
        
    }
    return 0;
}
