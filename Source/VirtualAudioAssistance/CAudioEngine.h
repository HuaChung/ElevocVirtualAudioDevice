#pragma once
#include "Builder.h"
#include "CBufferQueue.h"
#include <memory>
#include <Windows.h>

class CAudioEngine
{

public:
	CAudioEngine(const char* pszEvTaskName,int iSampleRate,int iChannels, int iRefChannels, int iFarSampleRate,int iDelayMS);
	~CAudioEngine();

public:

	void* GetEvTask();

	void AsyncWriteData(const float *pData, int iLen);
	void AsyncReadData(float* pData, int iLen);
	void AsyncWriteFarendData(const float *pData, int iLen);
	void SyncWriteAndReadData(const float* pNear, size_t ulNearSize, const float* pFar, size_t ulFarSize, float* pOutput);
	void SyncWriteAndReadData(void* pEvTask, const float* pNear, size_t ulNearSize, const float* pFar, size_t ulFarSize, float* pOutput);
	void* NewEvTask(const char* key,bool bIsAsync);

private:
	bool InitEngine();


private:
	std::shared_ptr<BaseModule> m_pInput_Module;
	std::shared_ptr<BaseModule> m_pOutput_Module;
	std::shared_ptr<BaseModule> m_pCns_Module;
	std::shared_ptr<BaseModule> m_pAec_Module;

	void* m_pEvtask;
	int m_iSampleRate;
	int m_iChannels;
	int m_iRefChannels;
	int m_iFarSampleRate;
	int m_iDelayMS;
private:

	Builder m_cBuilder;
	std::string m_strDevType;
	std::string m_strNsModuleName;
};
