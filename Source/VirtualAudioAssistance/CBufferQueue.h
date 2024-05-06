#pragma once

#include <memory.h>
#include <iostream>
#include <mutex>

#define NEAR_BLOCK_SIZE 480
#define NEAR_CHANNELS 2
#define BUFFER_FRAMES 4
const int BUFFER_SIZE = NEAR_BLOCK_SIZE * NEAR_CHANNELS * BUFFER_FRAMES * 100;
class CBufferQueue
{

public:
	CBufferQueue(int iChannels,int iQueueSize = BUFFER_SIZE);
	~CBufferQueue();

	bool Enqueue(void* pData, int size);
	int Dequeue(void* pData, int size = NEAR_BLOCK_SIZE * NEAR_CHANNELS * BUFFER_FRAMES);
	int GetBufferSize() const;
	void Clear();
	bool IsFull() const;
private:
	int          m_iBufferSize = 0;
	int          m_iChannels = 0;
	char*        m_pBuffer = nullptr;
	int			 m_iMaxSize = 0;
	std::mutex   m_mtx;
};

