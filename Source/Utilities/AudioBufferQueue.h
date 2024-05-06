#pragma once
#include "definitions.h"
#include <ntddk.h>

#define RENDER_START_EVENT_NAME L"\\BaseNamedObjects\\RenderStartEvent"
#define CAPTURE_START_EVENT_NAME L"\\BaseNamedObjects\\CaptureStartEvent"

#define RENDER_STOP_EVENT_NAME L"\\BaseNamedObjects\\RenderStopEvent"
#define CAPTURE_STOP_EVENT_NAME L"\\BaseNamedObjects\\CaptureStopEvent"

#define CAPTURE_BUFFER_QUEUE_SIZE 3840 * 100

extern bool g_bCurCaptureStart;
extern bool g_bCurRenderStart;

extern PKEVENT g_pRenderStartEvent;
extern PKEVENT g_pCaptureStartEvent;
extern HANDLE g_RenderStartEventHandle;
extern HANDLE g_CaptureStartEventHandle;

extern PKEVENT g_pRenderStoptEvent;
extern PKEVENT g_pCaptureStopEvent;
extern HANDLE g_RenderStopEventHandle;
extern HANDLE g_CaptureStopEventHandle;


typedef struct STAudioBufferQueue 
{
	BYTE* pBuffer;        // ָ��������������ָ��
	ULONG ulCapacity;     // ��������������
	ULONG ulDataSize;     // �������е�ǰ�洢�����ݴ�С
	KSPIN_LOCK Lock;      // ����ͬ����������
	ULONG ulDelaySize;    // �ӳٴ�С
} AudioBufferQueue, * pAudioBufferQueue;

extern AudioBufferQueue g_stCaptureAudioBufferQueue;

VOID InitializeBufferQueue(pAudioBufferQueue pQueue, ULONG ulInitialCapacity);
NTSTATUS EnqueueBuffer(pAudioBufferQueue pQueue, PVOID pData, ULONG ulDataLength);
NTSTATUS DequeueBuffer(pAudioBufferQueue pQueue, PVOID pOutputBuffer, ULONG ulRequestedSize, PULONG pulActualSize);
VOID ClearBufferQueue(pAudioBufferQueue pQueue);
ULONG GetBufferQueueSize(pAudioBufferQueue pQueue);
VOID DestroyByteQueue(pAudioBufferQueue pQueue);
