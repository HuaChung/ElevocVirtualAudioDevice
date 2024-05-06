#include "AudioBufferQueue.h"

AudioBufferQueue g_stCaptureAudioBufferQueue;

bool g_bCurCaptureStart = false;
bool g_bCurRenderStart = false;

PKEVENT g_pRenderStartEvent = NULL;
PKEVENT g_pCaptureStartEvent = NULL;
HANDLE g_RenderStartEventHandle = NULL;
HANDLE g_CaptureStartEventHandle = NULL;

PKEVENT g_pRenderStoptEvent = NULL;
PKEVENT g_pCaptureStopEvent = NULL;
HANDLE g_RenderStopEventHandle = NULL;
HANDLE g_CaptureStopEventHandle = NULL;

const int AUDIO_BUFFER_QUEUE_DELAY_SIZE = 0;

VOID InitializeBufferQueue(pAudioBufferQueue pQueue, ULONG ulInitialCapacity)
{
	pQueue->ulCapacity = ulInitialCapacity;
	pQueue->ulDataSize = 0;
	pQueue->ulDelaySize = AUDIO_BUFFER_QUEUE_DELAY_SIZE;
	pQueue->pBuffer = (BYTE*)ExAllocatePool2(POOL_FLAG_NON_PAGED, ulInitialCapacity, 'ABuf');
	KeInitializeSpinLock(&pQueue->Lock);
}

NTSTATUS EnqueueBuffer(pAudioBufferQueue pQueue, PVOID pData, ULONG ulDataLength)
{
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL oldIrql;
	KeAcquireSpinLock(&pQueue->Lock, &oldIrql);

	// ȷ���������㹻��������������
	if (pQueue->ulDataSize + ulDataLength > pQueue->ulCapacity)
	{
		DbgPrint("EnqueueBuffer: Buffer overflow\n");
		// ��չ�������߼�...
		KeReleaseSpinLock(&pQueue->Lock, oldIrql);
		return STATUS_BUFFER_OVERFLOW;
	}

	// �������ݵ�������
	RtlCopyMemory(pQueue->pBuffer + pQueue->ulDataSize, pData, ulDataLength);
	pQueue->ulDataSize += ulDataLength;
	
	KeReleaseSpinLock(&pQueue->Lock, oldIrql);
	return status;
}

NTSTATUS DequeueBuffer(pAudioBufferQueue pQueue, PVOID pOutputBuffer, ULONG ulRequestedSize, PULONG pulActualSize)
{
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL oldIrql;
	KeAcquireSpinLock(&pQueue->Lock, &oldIrql);

	if (ulRequestedSize == 0 || pOutputBuffer == NULL || pulActualSize == NULL)
	{
		DbgPrint("DequeueBuffer: Invalid parameter\r\n");
		KeReleaseSpinLock(&pQueue->Lock, oldIrql);
		return STATUS_INVALID_PARAMETER;
	}

	if (pQueue->ulDelaySize > pQueue->ulDataSize)
	{
		DbgPrint("DequeueBuffer : pQueue->ulDelaySize > pQueue->ulDataSize \r\n");
		*pulActualSize = 0;
		KeReleaseSpinLock(&pQueue->Lock, oldIrql);
		return STATUS_SUCCESS;
	}
	else
	{
		pQueue->ulDelaySize = 0;
	}

	// ����Ϊ��
	if (pQueue->ulDataSize == 0) 
	{ 
		DbgPrint("DequeueBuffer: Queue is empty\r\n");
		*pulActualSize = 0;
		KeReleaseSpinLock(&pQueue->Lock, oldIrql);
		return STATUS_SUCCESS;
	}

	// ����ʵ�ʿ��Ը��Ƶ����ݴ�С
	ULONG ulCopySize = min(ulRequestedSize, pQueue->ulDataSize);
	// �������ݵ����������
	RtlCopyMemory(pOutputBuffer, pQueue->pBuffer, ulCopySize);
	*pulActualSize = ulCopySize;
	// ��ʣ�������ƶ�������������ʼ��
	RtlMoveMemory(pQueue->pBuffer, pQueue->pBuffer + ulCopySize, pQueue->ulDataSize - ulCopySize);
	pQueue->ulDataSize -= ulCopySize;
	KeReleaseSpinLock(&pQueue->Lock, oldIrql);
	return status;
}


VOID ClearBufferQueue(pAudioBufferQueue pQueue)
{
	KIRQL oldIrql;
	KeAcquireSpinLock(&pQueue->Lock, &oldIrql);
	DbgPrint("ClearBufferQueue: Clearing buffer queue\n");
	pQueue->ulDataSize = 0;
	pQueue->ulDelaySize = AUDIO_BUFFER_QUEUE_DELAY_SIZE;
	KeReleaseSpinLock(&pQueue->Lock, oldIrql);
}

ULONG GetBufferQueueSize(pAudioBufferQueue pQueue)
{
	KIRQL oldIrql;
	KeAcquireSpinLock(&pQueue->Lock, &oldIrql);

	ULONG ulSize = pQueue->ulDataSize;

	KeReleaseSpinLock(&pQueue->Lock, oldIrql);
	return ulSize;

}

VOID DestroyByteQueue(pAudioBufferQueue pQueue)
{
	if (pQueue->pBuffer)
	{
		ExFreePoolWithTag(pQueue->pBuffer, 'ABuf');
		pQueue->pBuffer = NULL;
	}
}