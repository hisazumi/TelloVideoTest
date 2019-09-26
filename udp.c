#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <winsock2.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>
//#include "tello.h"
#include "udp.h"

static struct sockaddr_in recv_addr, send_addr, recv_addrv;
static int sock,sockv;

DWORD WINAPI ThreadProc(LPVOID lpParamater);
BOOL g_bExitThread = FALSE;
static HANDLE hThread = NULL;

void udp_open (void) {
	// start winsock and open a socket
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockv = socket(AF_INET, SOCK_DGRAM, 0);

    // make the sockets non-blocking
    u_long val=1;
    ioctlsocket(sock, FIONBIO, &val);

    // receiver (control)
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(9000);
    recv_addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr));

    // receiver (video)
	recv_addrv.sin_family = AF_INET;
	recv_addrv.sin_port = htons(11111);
	recv_addrv.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");
	bind(sockv, (struct sockaddr *)&recv_addrv, sizeof(recv_addrv));

    // sender
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(8889);
    send_addr.sin_addr.s_addr = inet_addr("192.168.10.1");
}

void udp_close (void) {
    close(sock);
    WSACleanup();
}

void udp_send (const char *cmd) {
    sendto(sock, cmd, strlen(cmd), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
}

void udp_poll (void) {
    char buf[2048];
    int size;
    memset(buf, 0, sizeof(buf));
    size = recv(sock, buf, sizeof(buf), 0);
    if (size > 0) {
    	//tello_recv( buf );
    }
}

/*
 * H264 decode
 */
static AVCodecContext        *context;
static AVFrame               *frame;
static AVCodec               *codec;
static AVCodecParserContext  *parser;

void video_init(void) {
	  //avcodec_register_all();

	  codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	  if (!codec) {
		  printf("cannot find decoder\n");
		  return;
	  }

	  context = avcodec_alloc_context3(codec);
	  if (!context) {
		  printf("cannot allocate context\n");
		  return;
	  }

	  if(codec->capabilities & AV_CODEC_CAP_TRUNCATED) {
	    context->flags |= AV_CODEC_FLAG_TRUNCATED;
	  }

	  int err = avcodec_open2(context, codec, NULL);
	  if (err < 0) {
		  printf("cannot open context");
		  return;
	  }

	  parser = av_parser_init(AV_CODEC_ID_H264);
	  if (!parser) {
	    printf("cannot init parser");
	    return;
	  }

	  frame = av_frame_alloc();
	  if (!frame) {
		  printf("cannot allocate frame");
		  return;
	  }
/*	  pkt = new AVPacket;
if (!pkt)
	    throw H264InitFailure("cannot allocate packet");
	  av_init_packet(pkt);
	}
*/
}

// video data receive
#define videoBufSize 80000
char packet_data[videoBufSize];

struct TelloVideo
{
	char frame[videoBufSize];
	char lastframe[videoBufSize];
	int  Ar_id;
};
struct TelloVideo tellov;	

int video_receive(void) {
	static TCHAR  szData[2018];			// receive buffer
	static int    bufp=0;
	static int    frame_length=0;
	int    nLen = sizeof(szData);
	int    nResult;

	memset((char *)szData, 0, sizeof(szData));
	nResult = recv(sockv, (char *)szData, nLen, 0);

	if (nResult == 0 || WSAGetLastError() == WSAECONNRESET) {
		printf("Video connection lost.\n");
		return -1;
	} else if (nResult > 0) {
		frame_length += nResult;
		if (frame_length < videoBufSize) {	// farme長が80000バイトより大きくなって（受信オーバフローで）なかったらコピー
			memcpy((char *)packet_data + bufp, (char *)szData, nResult);

			bufp += nResult;
			if (nResult != 1460) {	// end of frame?  if yes, process below
				printf("1460バイト以外の　最後の%dバイトを受信しました。\n", nResult);
	//			tellov.lastframe = h264_decode(packet_data);
				memcpy((char *)tellov.lastframe, (char *)packet_data, frame_length);
				printf("1 frame is received size=%d\n",frame_length);
				memset((char *)packet_data, 0, sizeof(packet_data));
				bufp = frame_length = 0;
			}
			else {
			}
		}
		else {	// farme長が80000バイトより大きくなった（受信オーバフロー）らデータを捨てる
			bufp = frame_length = 0;
			memset((char *)packet_data, 0, sizeof(packet_data));
			printf("オーバフローしたデータを捨てました\n");
		}
	}
	else {
		printf("receive failed. error=%d\n", nResult);
	}
	
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParamater)
{
	memset(packet_data, 0, sizeof(packet_data));	// clear buffer
	printf("UDP receive start! from port=%d\n",ntohs(recv_addrv.sin_port));		// print once

	while (!g_bExitThread) {
		if (video_receive() < 0) {
			break;
		}
		Sleep(1);
	}
	return 0;
}

void video_receive_start(void) {
	DWORD dwThreadId;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, &sockv, 0, &dwThreadId);
	return;
}



