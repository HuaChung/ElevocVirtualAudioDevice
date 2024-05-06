#pragma once

#include "CAudioEngine.h"
#include "IOCTL_Def.h"
#include "Speex/speex_resampler.h"
#include <Windows.h>
#include <mmreg.h>
#include <wrl/client.h>
#include <atomic>
#include <Audioclient.h>
#include <map>

using Microsoft::WRL::ComPtr;

class IMMDevice;
class mutex;
class condition_variable;
struct IAudioClient;
struct IAudioClient2;

class CVirtualAudioAssistance
{
	enum EDeviceType
	{
		Render = 0,
		Capture = 1,
		DeviceTypeMax
	};

	enum EWaveFormat
	{
		Channels = 2,
		SamplesPerSec = 48000,
		BitsPerSample = 32,
		FramesPerPacket = 480,
		CaptureAECDelay = 50
	};

public:
	CVirtualAudioAssistance();
	~CVirtualAudioAssistance();

	void Run();

private:
	int InitIOCTL();
	int InitEvent();
	int InitEngine();

	void ResetAudioEngine(std::shared_ptr<CAudioEngine>& pEngine ,const char* pszEvTaskName, int iSampleRate, int iChannels, int iRefChannels, int iFarSampleRate,int iDelayMS);
	int InitResampler();

	int InitDevice(int iType, DWORD dwStreamFlags);

	bool SendMicrophoneData(BYTE* pData, UINT32 uiPacketLength);

	void MonitorRenderStartThread();
	void MonitorRenderStopThread();
	void MonitorCaptureStartThread();
	void MonitorCaptureStopThread();

	void SpeakerThread();
	void MicrophoneThread();
	void CaptureReferData();
	void InitReferCapture();
	void InitVirtualRenderCapture();


	void ConvertPCM32toFloat(const int32_t* pPcmData, float* pFloatData, ULONG ulSize);
	void Float32ConvertToInt32(int32_t* pPcmData, float* pFloatData, ULONG ulSize);
private:

	ComPtr<IMMDevice> GetAudioDevice(const EDeviceType& eType, bool bIsVirtual = false);
	bool DeviceInitEx(ComPtr<IMMDevice>& pDevice, ComPtr<IAudioClient2>& pAudioClient, DWORD dwStreamFlags, int iType);

private:
	bool m_bInit;
	HANDLE m_hIOCTL;

	WAVEFORMATEX* m_pWaveFormats[DeviceTypeMax]{ nullptr };

	std::shared_ptr<CAudioEngine> m_pAudioEngines[DeviceTypeMax];

	ComPtr<IAudioCaptureClient> m_pAudioCaptureClient;
	ComPtr<IAudioRenderClient> m_pAudioRenderClient;
	ComPtr<IAudioClient2> m_pAudioClient[DeviceTypeMax]; // 0: Render, 1: Capture;
	ComPtr<IMMDevice> m_pAudioDevice[DeviceTypeMax]; // 0: Render, 1: Capture;

	WAVEFORMATEX* m_pAECWaveFormats = nullptr;
	ComPtr<IAudioCaptureClient> m_pAECSpeakerCaptureClient;
	ComPtr<IAudioClient2> m_pAECSpeakerAudioClient;
	ComPtr<IMMDevice> m_pAECAudioDevice;

	WAVEFORMATEX* m_pVirtualWaveFormats = nullptr;
	ComPtr<IAudioCaptureClient> m_pVirtualRenderCaptureClient;
	ComPtr<IAudioClient2> m_pVirtualAudioClient;
	ComPtr<IMMDevice> m_pVirtualAudioDevice;

	std::atomic<bool> m_bDeviceRunStatusFlags[2] = { false,false }; // 0: Render, 1: Capture

	std::map<LPCTSTR, HANDLE> m_mapEventName2Handle;
	LPCTSTR m_strEventNames[MAX_EVENT_COUNT] = { RENDER_START_EVENT_NAME, CAPTURE_START_EVENT_NAME, RENDER_STOP_EVENT_NAME,CAPTURE_STOP_EVENT_NAME };

	HANDLE m_AudioStreamEventHandles[DeviceTypeMax]{ nullptr };

	const char* m_szEngineEvTaskNames[DeviceTypeMax] = { "Speaker", "Microphone" };
	void* m_pEngineEvTasks[DeviceTypeMax]{ nullptr };

	std::mutex m_mtx[DeviceTypeMax]; // 0: Render, 1: Capture
	std::condition_variable m_cv[DeviceTypeMax];
	std::mutex m_MonitorMutex;

	int m_iFlag = 0;

	std::shared_ptr<CBufferQueue> m_pBuffQueue;
	std::shared_ptr<CBufferQueue> m_pReferQueue;
	SpeexResamplerState* m_pResamplerStates[2]{ nullptr };

	size_t m_ullReferPacketSize = 0;
	size_t m_ullVirtualRenderPacketSize = 0;  
	float m_pfFarInput[NEAR_BLOCK_SIZE * NEAR_CHANNELS * BUFFER_FRAMES * 2]{ 0 };

	bool m_bReferIsVAD = false;

	// --------------------------------------------

};