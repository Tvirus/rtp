#ifndef _RTSP_H_
#define _RTSP_H_


#include "typedef.h"



#define  RTSP_REQ_TYPE_UNKNOWN        0
#define  RTSP_REQ_TYPE_OPTION         1
#define  RTSP_REQ_TYPE_DESCRIBE       2
#define  RTSP_REQ_TYPE_SETUP          3
#define  RTSP_REQ_TYPE_PLAY           4
#define  RTSP_REQ_TYPE_TEARDOWN       5
#define  RTSP_REQ_TYPE_SET_PARAMETER  6
#define  RTSP_REQ_TYPE_GET_PARAMETER  7


#define  RTSP_MAX_URL_LEN             64
#define  RTSP_MAX_USER_AGENT_LEN      64


#define  RTSP_SUPPORTED_OPTIONS      "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY" /* 服务器支持的请求 */




typedef struct
{
    WORD32  dwSSRC;          /* 同步信源 synchronization source */
    WORD32  dwSessionID;
    WORD16  wRtpPort;
    WORD16  wRtcpPort;
    char   *pcServerName;
    char   *pcOptions;
    char   *pcDescribe
    char   *pcMulticastIp;

}T_RtspServerInfo;


typedef struct
{
    WORD32  dwSeqNum;
    WORD32  dwSessionID;
    WORD16  wRequestType;
    WORD16  wClientRtpPort;
    WORD16  wClientRtcpPort;
    BYTE    ucIsMulticast;
    BYTE    rsv;
    char    acURL[RTSP_MAX_URL_LEN];
    char    acUserAgent[RTSP_MAX_USER_AGENT_LEN];

}T_RtspReqInfo;




extern SWORD32 RTSP_ServerParseRequest(const char *pcRequest, const T_RtspSessionConfig *ptConfig, char *pcResponse, WORD16 wLen, T_RtspReqInfo *ptReqInfo);


#endif
