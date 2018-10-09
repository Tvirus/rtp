#include "rtp.h"



/*
    RTP头

 0               1               2               3
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|  CC   |M|     PT      |        sequence number        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           synchronization source (SSRC) identifier            |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|            contributing source (CSRC) identifiers             |
|                              ...                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

/* RTP头 */
typedef struct
{
    BYTE    CSRC:4;         /* 贡献源个数 Contributing source */
    BYTE    X:1;            /* 扩展标志，指示RTP报头后跟有一个扩展报头 */
    BYTE    P:1;            /* 填充标志，指示该报文的尾部填充一个或多个额外的八位组，它们不是负载的一部分 */
    BYTE    Version:2;      /* 版本 */

    BYTE    PayloadType:7;  /* 负载类型 */
    BYTE    M:1;            /* 对于视频，标记一帧的结束；对于音频，标记会话的开始 */

    WORD16  wSeqNum;        /* 序列号 sequence number */
    WORD32  dwTimeStamp;    /* 时间戳 */
    WORD32  dwSSRC;         /* 同步信源 synchronization source */

}T_RtpHeader;


/*
    FU分组

FU Indicator
 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+-+-+-+-+-+-+-+-+

FU Header
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|S|E|R|  Type   |
+---------------+
*/




static inline WORD16 HtonS(WORD16 x)
{
    WORD16 i = 7;

    if (*((BYTE*)&i))
    {
        x = (x >> 8) | (x << 8);
    }

    return x;
}

static inline WORD16 NtohS(WORD16 x)
{
    WORD16 i = 7;

    if (*((BYTE*)&i))
    {
        x = (x >> 8) | (x << 8);
    }

    return x;
}

static inline WORD32 HtonL(WORD32 x)
{
    WORD32 i = 7;

    if (*((BYTE*)&i))
    {
        x = (x >> 24) | ((x & 0xff0000) >> 8) | ((x & 0xff00) << 8) | (x << 24);
    }

    return x;
}

static inline WORD32 NtohL(WORD32 x)
{
    WORD32 i = 7;

    if (*((BYTE*)&i))
    {
        x = (x >> 24) | ((x & 0xff0000) >> 8) | ((x & 0xff00) << 8) | (x << 24);
    }

    return x;
}


static inline SWORD32 FillRtpHeader(T_RtpHeader *pHeader, const T_RtpFrameInfo *ptFrameInfo)
{
    memset(pHeader, 0, sizeof(T_RtpHeader));

    pHeader->Version     = 2;
    pHeader->wSeqNum     = HtonS(ptFrameInfo->wSeqNum);
    pHeader->dwTimeStamp = HtonL(ptFrameInfo->dwTimeStamp);
    pHeader->dwSSRC      = HtonL(ptFrameInfo->dwSSRC);

    if (ptFrameInfo->ucPayloadType & 0x80)
    {
        return -1;
    }
    pHeader->PayloadType = ptFrameInfo->ucPayloadType;

    return 0;
}


static SWORD32 H264AVC_SingleNal(const BYTE *pucData, WORD32 dwDataLen, BYTE *pucOutBuf, WORD32 dwOutBufLen, BYTE ucFrameType)
{
    BYTE ucNalHeader;

    if (5 >= dwDataLen)
    {
        return -1;
    }
    /* 略过每一帧开头的0x00000001起始位和NAL字节 */
    dwDataLen -= 5;
    pucData   += 5;


    if ((1 + dwDataLen) > pucOutBuf)
    {
        return -1;
    }

    if (0 == ucFrameType)      /* IDR帧 */
    {
        ucNalHeader = 0x65;               /* NRI=3, Type=5(IDR片) */
    }
    else if (1 == ucFrameType) /* I帧 */
    {
        ucNalHeader = 0x41;               /* NRI=2, Type=1(非IDR片) */
    }
    else if (2 == ucFrameType) /* P帧 */
    {
        ucNalHeader = 0x21;               /* NRI=1, Type=1(非IDR片) */
    }

    *pucOutBuf = ucNalHeader;
    memcpy(pucOutBuf + 1, pucData, dwDataLen);

    return 1 + dwDataLen;
}


static SWORD32 H264AVC_FUaStart(const BYTE *pucData, WORD32 dwDataLen, BYTE *pucOutBuf, WORD32 dwOutBufLen, BYTE ucFrameType)
{
    BYTE ucFUIndicator;
    BYTE ucFUHeader;

    if (5 >= dwDataLen)
    {
        return -1;
    }
    /* 略过每一帧开头的0x00000001起始位和NAL字节 */
    dwDataLen -= 5;
    pucData   += 5;


    if ((2 + dwDataLen) > dwOutBufLen)
    {
        return -1;
    }

    if (0 == ucFrameType)      /* IDR帧 */
    {
        ucFUIndicator = 0x7c;             /* NRI=3, Type=28(FU-A分片) */
        ucFUHeader    = 0x85;             /* S=1, Type=5(IDR片) */
    }
    else if (1 == ucFrameType) /* I帧 */
    {
        ucFUIndicator = 0x5c;             /* NRI=2, Type=28(FU-A分片) */
        ucFUHeader    = 0x81;             /* S=1, Type=1(非IDR片) */
    }
    else if (2 == ucFrameType) /* P帧 */
    {
        ucFUIndicator = 0x3c;             /* NRI=1, Type=28(FU-A分片) */
        ucFUHeader    = 0x81;             /* S=1, Type=1(非IDR片) */
    }

    *pucOutBuf       = ucFUIndicator;
    *(pucOutBuf + 1) = ucFUHeader;
    memcpy(pucOutBuf + 2, pucData, dwDataLen);

    return 2 + dwDataLen;
}


static SWORD32 H264AVC_FUaMiddle(const BYTE *pucData, WORD32 dwDataLen, BYTE *pucOutBuf, WORD32 dwOutBufLen, BYTE ucFrameType)
{
    BYTE ucFUIndicator;
    BYTE ucFUHeader;

    if ((2 + dwDataLen) > dwOutBufLen)
    {
        return -1;
    }

    if (0 == ucFrameType)      /* IDR帧 */
    {
        ucFUIndicator = 0x7c;             /* NRI=3, Type=28(FU-A分片) */
        ucFUHeader    = 0x05;             /* Type=5(IDR片) */
    }
    else if (1 == ucFrameType) /* I帧 */
    {
        ucFUIndicator = 0x5c;             /* NRI=2, Type=28(FU-A分片) */
        ucFUHeader    = 0x01;             /* Type=1(非IDR片) */
    }
    else if (2 == ucFrameType) /* P帧 */
    {
        ucFUIndicator = 0x3c;             /* NRI=1, Type=28(FU-A分片) */
        ucFUHeader    = 0x01;             /* Type=1(非IDR片) */
    }

    *pucOutBuf       = ucFUIndicator;
    *(pucOutBuf + 1) = ucFUHeader;
    memcpy(pucOutBuf + 2, pucData, dwDataLen);

    return 2 + dwDataLen;
}


static SWORD32 H264AVC_FUaEnd(const BYTE *pucData, WORD32 dwDataLen, BYTE *pucOutBuf, WORD32 dwOutBufLen, BYTE ucFrameType)
{
    BYTE ucFUIndicator;
    BYTE ucFUHeader;

    if ((2 + dwDataLen) > dwOutBufLen)
    {
        return -1;
    }

    if (0 == ucFrameType)      /* IDR帧 */
    {
        ucFUIndicator = 0x7c;             /* NRI=3, Type=28(FU-A分片) */
        ucFUHeader    = 0x45;             /* E=1, Type=5(IDR片) */
    }
    else if (1 == ucFrameType) /* I帧 */
    {
        ucFUIndicator = 0x5c;             /* NRI=2, Type=28(FU-A分片) */
        ucFUHeader    = 0x41;             /* E=1, Type=1(非IDR片) */
    }
    else if (2 == ucFrameType) /* P帧 */
    {
        ucFUIndicator = 0x3c;             /* NRI=1, Type=28(FU-A分片) */
        ucFUHeader    = 0x41;             /* E=1, Type=1(非IDR片) */
    }

    *pucOutBuf       = ucFUIndicator;
    *(pucOutBuf + 1) = ucFUHeader;
    memcpy(pucOutBuf + 2, pucData, dwDataLen);

    return 2 + dwDataLen;
}


static SWORD32 EncodeH264AVC(const BYTE *pucData, WORD32 dwDataLen, BYTE *pucOutBuf, WORD32 dwOutBufLen, const T_RtpFrameInfo *ptFrameInfo)
{

    /* 新的一帧 */
    if (ptFrameInfo->ucNewFrame)
    {
        /* 单个NAL包 */
        if (ptFrameInfo->ucEndOfFrame)
        {
            return H264AVC_SingleNal(pucData, dwDataLen, pucOutBuf, dwOutBufLen, ptFrameInfo->ucFrameType);
        }
        /* 分片包 */
        else
        {
            return H264AVC_FUaStart(pucData, dwDataLen, pucOutBuf, dwOutBufLen, ptFrameInfo->ucFrameType);
        }
    }
    else
    {
        /* 分片结束 */
        if (ptFrameInfo->ucEndOfFrame) //交换顺序？？？？？
        {
            return H264AVC_FUaEnd(pucData, dwDataLen, pucOutBuf, dwOutBufLen, ptFrameInfo->ucFrameType);
        }
        /* 分片中间包 */
        else
        {
            return H264AVC_FUaMiddle(pucData, dwDataLen, pucOutBuf, dwOutBufLen, ptFrameInfo->ucFrameType);
        }
    }
}


SWORD32 RTP_Encode(const BYTE *pucData, WORD32 dwDataLen, BYTE *pucOutBuf, WORD32 dwOutBufLen, const T_RtpFrameInfo *ptFrameInfo)
{
    T_RtpHeader tRtpHeader = {0};

    if (NULL == pucOutBuf || NULL == ptFrameInfo)
        return -1;

    if (sizeof(tRtpHeader) >= dwOutBufLen)
    {
        return -1;
    }
    if (0 > FillRtpHeader(&tRtpHeader, ptFrameInfo))
    {
        return -1;
    }

    /* 视频 */
    if (RTP_AVTYPE_VIDEO == ptFrameInfo->ucAvType)
    {
        /* 一帧的结束 */
        if (ptFrameInfo->ucEndOfFrame)
        {
            tRtpHeader.M = 1;
        }
        else
        {
            tRtpHeader.M = 0;
        }

        memcpy(pucOutBuf, &tRtpHeader, sizeof(tRtpHeader));
        pucOutBuf   += sizeof(tRtpHeader);
        dwOutBufLen -= sizeof(tRtpHeader);

        if (RTP_PAYLOAD_FORMAT_H264AVC == ptFrameInfo->ucPayloadFormat)
        {
            return EncodeH264AVC(pucData, dwDataLen, pucOutBuf, dwOutBufLen, ptFrameInfo) + sizeof(tRtpHeader);
        }
        else
        {
            return -1;
        }
    }
    /* 音频 */
    else if (RTP_AVTYPE_AUDIO == ptFrameInfo->ucAvType)
    {
        if (RTP_PAYLOAD_FORMAT_PCM16L == ptFrameInfo->ucPayloadFormat)
        {
        }
        else if (RTP_PAYLOAD_FORMAT_PCMU == ptFrameInfo->ucPayloadFormat)
        {
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }

    return 0;
}
