#include "rtsp.h"



#define  RTSP_REQUEST_OPTION         "OPTIONS"
#define  RTSP_REQUEST_DESCRIBE       "DESCRIBE"
#define  RTSP_REQUEST_SETUP          "SETUP"
#define  RTSP_REQUEST_PLAY           "PLAY"
#define  RTSP_REQUEST_TEARDOWN       "TEARDOWN"
#define  RTSP_REQUEST_SET_PARAMETER  "SET_PARAMETER"
#define  RTSP_REQUEST_GET_PARAMETER  "SET_PARAMETER"

#define  RTSP_RESPONSE_OK            "RTSP/1.0 200 OK"

#define  RTSP_SEQ_HEAD               "CSeq:"
#define  RTSP_URL_HEAD               "rtsp://"
#define  RTSP_USER_AGENT_HEAD        "User-Agent:"
#define  RTSP_CLIENT_PORT_HEAD       "client_port="
#define  RTSP_SESSION_HEAD           "Session:"


static SWORD32 GetIntValue(const char *pcString, const char *pcKey, WORD32 *pdwValue)
{
    char *pcStart = NULL;

    pcStart = strstr(pcString, pcKey);
    if (NULL == pcStart)
    {
        Println(ERROR_LEVEL, "There is no \"%s\" in the request !", pcKey);
        return -1;
    }

    pcStart += strlen(pcKey);
    while (1)
    {
        if (('0' <= *pcStart) && ('9' >= *pcStart))
            break;
        else if (' ' == *pcStart)
            pcStart++;
        else
        {
            Println(ERROR_LEVEL, "There is no \"%s\" value in the request !", pcKey);
            return -1;
        }
    }

    *pdwValue = (WORD32)atoi(pcStart);
    return 0;
}


static SWORD32 GetURL(const char *pcString, char *pcOutBuf, WORD16 wBufLen)
{
    char   *pcStart = NULL;
    char   *pcEnd   = NULL;
    WORD16  wLen;

    pcStart = strstr(pcString, RTSP_URL_HEAD);
    if (NULL == pcStart)
    {
        Println(ERROR_LEVEL, "There is no URL(%s) in the request !", RTSP_URL_HEAD);
        return -1;
    }

    pcEnd = pcStart + sizeof(RTSP_URL_HEAD) - 1;
    while (1)
    {
        if ((' '  == *pcEnd) || ('\r' == *pcEnd) || ('\n' == *pcEnd) || ('\0' == *pcEnd))
            break;
        pcEnd++;
    }

    wLen = pcEnd - pcStart + 1;
    if (wLen <= wBufLen)
    {
        memcpy(pcOutBuf, pcStart, wLen);
        pcOutBuf[wLen - 1] = 0;
        return (SWORD32)wLen;
    }
    else
    {
        Println(ERROR_LEVEL, "The length[%u] of URL exceeds the max[%u] !", wLen, wBufLen);
        return -((SWORD32)wLen);
    }
}


static SWORD32 GetUserAgent(const char *pcString, char *pcOutBuf, WORD16 wBufLen)
{
    char   *pcStart = NULL;
    char   *pcEnd   = NULL;
    WORD16  wLen;

    pcStart = strstr(pcString, RTSP_USER_AGENT_HEAD);
    if (NULL == pcStart)
    {
        Println(ERROR_LEVEL, "There is no user agent(%s) in the request !", RTSP_USER_AGENT_HEAD);
        return -1;
    }

    pcEnd = pcStart + sizeof(RTSP_USER_AGENT_HEAD) - 1;
    while (1)
    {
        if (('\r' == *pcEnd) || ('\n' == *pcEnd) || ('\0' == *pcEnd))
            break;
        pcEnd++;
    }

    wLen = pcEnd - pcStart + 1;
    if (wLen <= wBufLen)
    {
        memcpy(pcOutBuf, pcStart, wLen);
        pcOutBuf[wLen - 1] = 0;
        return (SWORD32)wLen;
    }
    else
    {
        Println(ERROR_LEVEL, "The length[%u] of URL exceeds the max[%u] !", wLen, wBufLen);
        return -((SWORD32)wLen);
    }
}


static SWORD32 GetClientPort(const char *pcString, WORD16 *pwRtpPort, WORD16 *pwRtcpPort)
{
    char *pcStart = NULL;

    pcStart = strstr(pcString, RTSP_CLIENT_PORT_HEAD);
    if (NULL == pcStart)
    {
        Println(DEBUG_LEVEL, "There is no client port(%s) in the request", RTSP_CLIENT_PORT_HEAD);
        return -1;
    }

    pcStart += sizeof(RTSP_CLIENT_PORT_HEAD) - 1;
    while (1)
    {
        if (('0' <= *pcStart) && ('9' >= *pcStart))
            break;
        else if (' ' == *pcStart)
            pcStart++;
        else
        {
            Println(ERROR_LEVEL, "There is no port num(%s) in the request !", RTSP_CLIENT_PORT_HEAD);
            return -1;
        }
    }

    *pwRtpPort = (WORD16)atoi(pcStart);

    pcStart++;
    while (1)
    {
        if (('0' <= *pcStart) && ('9' >= *pcStart))
            pcStart++;
        else if ('-' == *pcStart)
        {
            break;
        }
        else
        {
            Println(ERROR_LEVEL, "There is no port num(%s) in the request !!", RTSP_CLIENT_PORT_HEAD);
            return -1;
        }
    }

    pcStart++;
    if (('0' <= *pcStart) && ('9' >= *pcStart))
    {
        *pwRtcpPort = (WORD16)atoi(pcStart);
        return 0;
    }
    else
    {
        Println(ERROR_LEVEL, "There is no port num(%s) in the request !!!", RTSP_CLIENT_PORT_HEAD);
        return -1;
    }
}



static SWORD32 ParseOptionsRequest(const char              *pcRequest,
                                   const T_RtspServerInfo  *ptServerInfo,
                                   char                    *pcResBuf,
                                   WORD16                   wBufLen,
                                   T_RtspReqInfo           *ptReqInfo)
{
    int iResLen;

    iResLen = snprintf(pcResBuf, (size_t)wBufLen, "%s\r\nCSeq: %u\r\n%s\r\n\r\n",
                                                    RTSP_RESPONSE_OK,
                                                    ptReqInfo->dwSeqNum,
                                                    RTSP_SUPPORTED_OPTIONS);

    if (unlikely(0 > iResLen))
    {
        Println(ERROR_LEVEL, "snprintf return error[%d] !", iResLen);
        return -1;
    }
    iResLen++;
    if (iResLen <= wBufLen)
    {
        return (SWORD32)iResLen;
    }
    else
    {
        Println(ERROR_LEVEL, "The length[%d] of response exceeds the max[%u] !", iResLen, wBufLen);
        return -((SWORD32)iResLen);
    }
}


static SWORD32 ParseDescribeRequest(const char              *pcRequest,
                                    const T_RtspServerInfo  *ptServerInfo,
                                    char                    *pcResBuf,
                                    WORD16                   wBufLen,
                                    T_RtspReqInfo           *ptReqInfo)
{
    int     iResLen;
    size_t  szDescLen;

    szDescLen = strlen(ptServerInfo->pcDescribe);

    iResLen = snprintf(pcResBuf, (size_t)wBufLen, "%s\r\nCSeq: %u\r\nContent-Base: %s\r\nContent-Type: application/sdp\r\nContent-Length: %zu\r\n\r\n",
                                                    RTSP_RESPONSE_OK,
                                                    ptReqInfo->dwSeqNum,
                                                    ptReqInfo->acURL,
                                                    szDescLen);

    if (unlikely(0 > iResLen))
    {
        Println(ERROR_LEVEL, "snprintf return error[%d] !", iResLen);
        return -1;
    }

    pcResBuf += iResLen;
    iResLen  += szDescLen + 1;
    if (iResLen <= wBufLen)
    {
        memcpy(pcResBuf, ptServerInfo->pcDescribe, szDescLen);
        pcResBuf[szDescLen] = 0;
        return (SWORD32)iResLen;
    }
    else
    {
        Println(ERROR_LEVEL, "The length[%d] of response exceeds the max[%u] !", iResLen, wBufLen);
        return -((SWORD32)iResLen);
    }
}


static SWORD32 ParseSetupRequest(const char              *pcRequest,
                                 const T_RtspServerInfo  *ptServerInfo,
                                 char                    *pcResBuf,
                                 WORD16                   wBufLen,
                                 T_RtspReqInfo           *ptReqInfo)
{
    WORD16  wRtpPort, wRtcpPort;
    int     iResLen;
    size_t  szDescLen;

    if (0 <= GetClientPort(pcRequest, &wRtpPort, &wRtcpPort))
    {
        ptReqInfo->wClientRtpPort  = wRtpPort;
        ptReqInfo->wClientRtcpPort = wRtcpPort;
    }
    else
    {
        ptReqInfo->wClientRtpPort  = ptServerInfo->wRtpPort;
        ptReqInfo->wClientRtcpPort = ptServerInfo->wRtcpPort;
    }

    if (strstr(pcRequest, "unicast"))
    {
        iResLen = snprintf(pcResBuf, (size_t)wBufLen, "%s\r\nCSeq: %u\r\nSession: %u\r\nTransport: RTP/AVP;unicast;client_port=%u-%u;server_port=%u-%u;ssrc=%u\r\n\r\n",
                                                        RTSP_RESPONSE_OK,
                                                        ptReqInfo->dwSeqNum,
                                                        ptServerInfo->dwSessionID,
                                                        ptReqInfo->wClientRtpPort,
                                                        ptReqInfo->wClientRtcpPort,
                                                        ptServerInfo->wRtpPort,
                                                        ptServerInfo->wRtcpPort,
                                                        ptServerInfo->dwSSRC);
    }
    else
    {
        iResLen = snprintf(pcResBuf, (size_t)wBufLen, "%s\r\nCSeq: %u\r\nSession: %u\r\nTransport: RTP/AVP;multicast;destination=%s;port=%u-%u;ssrc=%u\r\n\r\n",
                                                        RTSP_RESPONSE_OK,
                                                        ptReqInfo->dwSeqNum,
                                                        ptServerInfo->dwSessionID,
                                                        ptServerInfo->pcMulticastIp,
                                                        ptServerInfo->wRtpPort,
                                                        ptServerInfo->wRtcpPort,
                                                        ptServerInfo->dwSSRC);
    }

    if (unlikely(0 > iResLen))
    {
        Println(ERROR_LEVEL, "snprintf return error[%d] !", iResLen);
        return -1;
    }
    iResLen ++;
    if (iResLen <= wBufLen)
    {
        return (SWORD32)iResLen;
    }
    else
    {
        Println(ERROR_LEVEL, "The length[%d] of response exceeds the max[%u] !", iResLen, wBufLen);
        return -((SWORD32)iResLen);
    }
}


static SWORD32 ParsePlayRequest(const char              *pcRequest,
                                const T_RtspServerInfo  *ptServerInfo,
                                char                    *pcResBuf,
                                WORD16                   wBufLen,
                                T_RtspReqInfo           *ptReqInfo)
{
    int iResLen;

    if (0 > GetIntValue(pcRequest, RTSP_SESSION_HEAD, &(ptReqInfo->dwSessionID)))
        return -1;

    iResLen = snprintf(pcResBuf, (size_t)wBufLen, "%s\r\nCSeq: %u\r\nRange: npt=0.000-\r\nSession: %u\r\nRTP-Info: url=%s\r\n\r\n",
                                                    RTSP_RESPONSE_OK,
                                                    ptReqInfo->dwSeqNum,
                                                    ptReqInfo->dwSessionID
                                                    ptReqInfo->acURL);

    if (unlikely(0 > iResLen))
    {
        Println(ERROR_LEVEL, "snprintf return error[%d] !", iResLen);
        return -1;
    }

    iResLen++;
    if (iResLen <= wBufLen)
    {
        return (SWORD32)iResLen;
    }
    else
    {
        Println(ERROR_LEVEL, "The length[%d] of response exceeds the max[%u] !", iResLen, wBufLen);
        return -((SWORD32)iResLen);
    }
}


static SWORD32 ParseTeardownRequest(const char              *pcRequest,
                                    const T_RtspServerInfo  *ptServerInfo,
                                    char                    *pcResBuf,
                                    WORD16                   wBufLen,
                                    T_RtspReqInfo           *ptReqInfo)
{
    int iResLen;

    if (0 > GetIntValue(pcRequest, RTSP_SESSION_HEAD, &(ptReqInfo->dwSessionID)))
        return -1;

    iResLen = snprintf(pcResBuf, (size_t)wBufLen, "%s\r\nCSeq: %u\r\nRange: npt=0.000-\r\nSession: %u\r\nRTP-Info: url=%s\r\n\r\n",
                                                    RTSP_RESPONSE_OK,
                                                    ptReqInfo->dwSeqNum,
                                                    ptReqInfo->dwSessionID
                                                    ptReqInfo->acURL);

    if (unlikely(0 > iResLen))
    {
        Println(ERROR_LEVEL, "snprintf return error[%d] !", iResLen);
        return -1;
    }

    iResLen++;
    if (iResLen <= wBufLen)
    {
        return (SWORD32)iResLen;
    }
    else
    {
        Println(ERROR_LEVEL, "The length[%d] of response exceeds the max[%u] !", iResLen, wBufLen);
        return -((SWORD32)iResLen);
    }
}


SWORD32 RTSP_ServerParseRequest(const char              *pcRequest,
                                const T_RtspServerInfo  *ptServerInfo,
                                char                    *pcResBuf,
                                WORD16                   wBufLen,
                                T_RtspReqInfo           *ptReqInfo)
{
    if (NULL == pcRequest || NULL == ptServerInfo || NULL == pcResBuf || NULL == ptReqInfo || 0 == wBufLen)
    {
        Println(ERROR_LEVEL, "Input error !");
        return -1;
    }
    if (NULL == ptServerInfo->pcServerName || NULL == ptServerInfo->pcOptions || NULL == ptServerInfo->pcDescribe)
    {
        Println(ERROR_LEVEL, "Input ptServerInfo error !");
        return -1;
    }

    
    /* 获取请求序号 */
    if (0 > GetIntValue(pcRequest, RTSP_SEQ_HEAD, &(ptReqInfo->dwSeqNum)))
        return -1;

    /* 获取URL */
    if (0 > GetURL(pcRequest, ptReqInfo->acURL, sizeof(ptReqInfo->acURL)))
        return -1;

    /* 获取客户端名称 */
    if (0 > GetUserAgent(pcRequest, ptReqInfo->acUserAgent, sizeof(ptReqInfo->acUserAgent)))
        ptReqInfo->acUserAgent[0] = 0;


    /* 解析Option请求 */
    if (0 == memcmp(pcRequest, RTSP_REQUEST_OPTION, sizeof(RTSP_REQUEST_OPTION)))
    {
        pkt_info->pkt_type = RTSP_REQ_TYPE_OPTION;
        return ParseOptionsRequest(pcRequest, ptServerInfo, pcResBuf, wBufLen, ptReqInfo);
    }
    /* 解析describe请求 */
    else if (0 == memcmp(pcRequest, RTSP_REQUEST_DESCRIBE, sizeof(RTSP_REQUEST_DESCRIBE)))
    {
        pkt_info->pkt_type = RTSP_REQ_TYPE_DESCRIBE;
        return ParseDescribeRequest(pcRequest, ptServerInfo, pcResBuf, wBufLen, ptReqInfo);
    }
    /* 解析setup请求 */
    else if (0 == memcmp(pcRequest, RTSP_REQUEST_SETUP, sizeof(RTSP_REQUEST_SETUP)))
    {
        pkt_info->pkt_type = RTSP_REQ_TYPE_SETUP;
        return ParseSetupRequest(pcRequest, ptServerInfo, pcResBuf, wBufLen, ptReqInfo);
    }
    /* 解析play请求 */
    else if (0 == memcmp(pcRequest, RTSP_REQUEST_PLAY, sizeof(RTSP_REQUEST_PLAY)))
    {
        pkt_info->pkt_type = RTSP_REQ_TYPE_PLAY;
        return ParsePlayRequest(pcRequest, ptServerInfo, pcResBuf, wBufLen, ptReqInfo);
    }
    /* 解析teardown请求 */
    else if (0 == memcmp(pcRequest, RTSP_REQUEST_TEARDOWN, sizeof(RTSP_REQUEST_TEARDOWN)))
    {
        pkt_info->pkt_type = RTSP_REQ_TYPE_TEARDOWN;
        return ParseTeardownRequest(pcRequest, ptServerInfo, pcResBuf, wBufLen, ptReqInfo);
    }
    else
    {
        Println(ERROR_LEVEL, "Unknown request !");
        return -1;
    }
}

