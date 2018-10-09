#ifndef _RTP_H_
#define _RTP_H_


#include "typedef.h"



/* 视频、音频 */
#define  RTP_AVTYPE_VIDEO   0
#define  RTP_AVTYPE_AUDIO   1

/* 编码格式 */
#define  RTP_PAYLOAD_FORMAT_H264AVC  0
#define  RTP_PAYLOAD_FORMAT_PCM16L   1
#define  RTP_PAYLOAD_FORMAT_PCMU     2







typedef struct
{
    WORD32  dwSSRC;          /* 同步信源 synchronization source */
    WORD32  dwTimeStamp;     /* 时间戳 */
    WORD16  wSeqNum;         /* 序列号 sequence number */
    BYTE    ucPayloadType;   /* 负载类型 */
    BYTE    ucAvType;        /* 视频或音频 */
    BYTE    ucPayloadFormat; /* 编码格式 */
    BYTE    ucNewFrame;      /* (视频) 一帧的开始 */
    BYTE    ucEndOfFrame;    /* (视频) 一帧的结束 */
    BYTE    ucFrameType;     /* (视频)  0:IDR帧  1:I帧  2:P帧 */

}T_RtpFrameInfo;




extern SWORD32 RTP_Encode(const BYTE *pucData, WORD32 dwDataLen, BYTE *pucOutBuf, WORD32 dwOutBufLen, const T_RtpFrameInfo *ptFrameInfo);



#endif
