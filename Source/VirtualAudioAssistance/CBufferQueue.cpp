#include "CBufferQueue.h"


CBufferQueue::CBufferQueue(int iChannels, int iQueueSize) : m_iChannels(iChannels), m_iBufferSize(0), m_iMaxSize(iQueueSize)
{
	m_pBuffer = new char[m_iMaxSize] {0};
}


CBufferQueue::~CBufferQueue()
{
	if (m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = nullptr;
	}

}

bool CBufferQueue::Enqueue(void* pData, int iSize)
{
	if (!pData || iSize <= 0 || !m_pBuffer)
	{
		return false;
	}

	std::unique_lock<std::mutex>lock(m_mtx);
	//std::cout << "enqueue iSize: " << iSize << std::endl;
	if (m_iBufferSize + iSize > m_iMaxSize)
	{
		int iSpaceNeeded = m_iBufferSize + iSize - m_iMaxSize;
		std::cout << "Buffer is full, adjusting by removing " << iSpaceNeeded << " bytes." << std::endl;

		// 调整缓冲区，删除旧数据
		memmove(m_pBuffer, m_pBuffer + iSpaceNeeded, m_iBufferSize - iSpaceNeeded);
		m_iBufferSize -= iSpaceNeeded;  // 更新缓冲区大小
	}

	memcpy(m_pBuffer + m_iBufferSize, pData, iSize);
	m_iBufferSize += iSize;

	return true;
}

int CBufferQueue::Dequeue(void* pData, int iSize)
{
	if (!pData || iSize <= 0)
	{
		return 0;
	}

	std::unique_lock<std::mutex>lock(m_mtx);
	if (m_iBufferSize <= 0)
	{
		return 0;
	}

	if (m_iBufferSize == iSize)
	{
		memcpy(pData, m_pBuffer, iSize);
		m_iBufferSize -= iSize;
		return iSize;
	}
	else if (m_iBufferSize > iSize)
	{
		memcpy(pData, m_pBuffer, iSize);
		memmove(m_pBuffer, m_pBuffer + iSize, (m_iBufferSize - iSize));
		m_iBufferSize -= iSize;
		return iSize;
	}
	else if (m_iBufferSize < iSize)
	{
		memcpy(pData, m_pBuffer, m_iBufferSize);
		int tmp = m_iBufferSize;
		m_iBufferSize = 0;
		return tmp;
	}
	return 0;
}

int CBufferQueue::GetBufferSize() const
{
	return m_iBufferSize;
}

void CBufferQueue::Clear()
{
	std::cout << "Clear buffer" << std::endl;
	m_iBufferSize = 0;
}

bool CBufferQueue::IsFull() const
{
	return m_iBufferSize >= m_iMaxSize;
}