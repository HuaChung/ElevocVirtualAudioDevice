#include "CVirtualAudioAssistance.h"
#include <initguid.h>
#include <iostream>
#include <propsys.h>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <thread>
#include <algorithm>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <iostream>

HRESULT CompareDevices(ComPtr<IMMDevice> pDevice1, ComPtr<IMMDevice> pDevice2, bool& bIsEqual)
{
	bIsEqual = false; // Default to not equal

	if (!pDevice1 || !pDevice2)
	{
		return E_POINTER; // Null pointer passed, cannot compare
	}

	LPWSTR wszDeviceID1 = nullptr;
	LPWSTR wszDeviceID2 = nullptr;

	// Get the device ID for the first device
	HRESULT hr = pDevice1->GetId(&wszDeviceID1);
	if (FAILED(hr))
	{
		return hr; // Failed to get device ID
	}

	// Get the device ID for the second device
	hr = pDevice2->GetId(&wszDeviceID2);
	if (FAILED(hr))
	{
		CoTaskMemFree(wszDeviceID1); // Clean up the first ID
		return hr; // Failed to get device ID
	}

	// Compare the device IDs
	bIsEqual = (wcscmp(wszDeviceID1, wszDeviceID2) == 0);

	// Free memory allocated for device IDs
	CoTaskMemFree(wszDeviceID1);
	CoTaskMemFree(wszDeviceID2);

	return S_OK; // Successfully compared
}

using namespace std;

void SetThreadAffinity(std::thread& th, DWORD_PTR mask)
{
	auto handle = th.native_handle(); // 获取原生线程句柄
	if (SetThreadAffinityMask(handle, mask) == 0)
	{
		DWORD error = GetLastError();
		std::cerr << "Failed to set thread affinity, error code: " << error << std::endl;
	}
}

CVirtualAudioAssistance::CVirtualAudioAssistance() :
	m_hIOCTL(INVALID_HANDLE_VALUE), m_bInit(false)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (0 != InitIOCTL())
	{
		cout << "Init IOCTL failed!" << endl;
		return;
	}

	if (0 != InitEvent())
	{
		cout << "Init event failed!" << endl;
		return;
	}

	DWORD dwType = 0;
	for (int i = 0; i < DeviceTypeMax; i++)
	{
		if (0 != InitDevice(i, dwType))
		{
			cout << "Init Device failed! index = " << i << endl;
			return;
		}
	}

	InitVirtualRenderCapture();
	InitReferCapture();
	if (0 != InitEngine())
	{
		cout << "Init Engine failed!" << endl;
		return;
	}

	m_pReferQueue = make_shared<CBufferQueue>(2, 3840 * 3);


	if (0 != InitResampler())
	{
		return;
	}


	m_bInit = true;
}

CVirtualAudioAssistance::~CVirtualAudioAssistance()
{
	if (m_hIOCTL != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hIOCTL);
	}

	for (int i = 0; i < DeviceTypeMax; i++)
	{
		if (m_pWaveFormats[i])
		{
			delete m_pWaveFormats[i];
			m_pWaveFormats[i] = nullptr;
		}

		if (m_AudioStreamEventHandles[i])
		{
			CloseHandle(m_AudioStreamEventHandles[i]);
		}

		if (m_pResamplerStates[i])
		{
			speex_resampler_destroy(m_pResamplerStates[i]);
			m_pResamplerStates[i] = nullptr;
		}

	}

	for (auto& it : m_mapEventName2Handle)
	{
		if (it.second)
		{
			CloseHandle(it.second);
		}
	}


	CoUninitialize();
}

void CVirtualAudioAssistance::Run()
{
	if (!m_bInit)
	{
		cout << "Init failed!" << endl;
		return;
	}

	if (m_hIOCTL == INVALID_HANDLE_VALUE)
	{
		cout << "Invalid handle!" << endl;
		return;
	}

	//thread tSpeaker(&CVirtualAudioAssistance::SpeakerThread, this);

	std::vector<std::thread> vecThreads;

	vecThreads.emplace_back(&CVirtualAudioAssistance::MicrophoneThread, this);
	vecThreads.emplace_back(&CVirtualAudioAssistance::SpeakerThread, this);
	vecThreads.emplace_back(&CVirtualAudioAssistance::MonitorRenderStartThread, this);
	vecThreads.emplace_back(&CVirtualAudioAssistance::MonitorRenderStopThread, this);
	vecThreads.emplace_back(&CVirtualAudioAssistance::MonitorCaptureStartThread, this);
	vecThreads.emplace_back(&CVirtualAudioAssistance::MonitorCaptureStopThread, this);

	// 假设系统的CPU核心数
	DWORD_PTR uiSystemCores = std::thread::hardware_concurrency();

	// 根据核心数计算每个核心应分配的线程数
	size_t ulThreadsPerCore = (vecThreads.size() + uiSystemCores - 1) / uiSystemCores;

	DWORD_PTR mask = 0;
	for (size_t i = 0; i < vecThreads.size(); ++i)
	{
		// 生成亲和性掩码，平均分配线程
		mask = 1 << (i / ulThreadsPerCore);

		// 限制mask不超过系统核心数
		mask = mask % (1 << uiSystemCores);

		SetThreadAffinity(vecThreads[i], mask);
	}

	for (int i = 0; i < 2; ++i)
	{
		if (!SetThreadPriority(vecThreads[i].native_handle(), THREAD_PRIORITY_HIGHEST))
		{
			// 如果设置线程优先级失败，输出错误信息
			std::cerr << "Failed to set thread priority. Error: " << GetLastError() << std::endl;
		}
	}

	// 等待所有线程完成
	for (auto& it : vecThreads)
	{
		if (it.joinable())
		{
			it.join();
		}
	}
}

int CVirtualAudioAssistance::InitIOCTL()
{
	m_hIOCTL = CreateFile(
		SYMBOL_LINK, // 符号链接名称
		GENERIC_ALL, // 需要的访问权限
		FILE_SHARE_READ | FILE_SHARE_WRITE, // 共享模式
		NULL, // 安全属性
		OPEN_EXISTING, // 创建方式
		FILE_ATTRIBUTE_NORMAL, // 文件属性
		NULL); // 模板文件的句柄

	if (m_hIOCTL == INVALID_HANDLE_VALUE)
	{
		return -1;
	}

	return 0;
}

int CVirtualAudioAssistance::InitEvent()
{
	DWORD dwBytesReturned = 0;
	BOOL bResult = DeviceIoControl(
		m_hIOCTL, // 设备句柄
		IOCTL_INIT_EVENT, // 控制码
		NULL, // 输入缓冲区
		0, // 输入缓冲区大小
		NULL, // 输出缓冲区
		0, // 输出缓冲区大小
		&dwBytesReturned, // 实际输出缓冲区大小
		NULL); // 重叠结构

	if (bResult)
	{

		for (int i = 0; i < MAX_EVENT_COUNT; i++)
		{
			m_mapEventName2Handle[m_strEventNames[i]] = OpenEvent(SYNCHRONIZE, FALSE, m_strEventNames[i]);
			if (m_mapEventName2Handle[m_strEventNames[i]] == NULL)
			{
				cout << "OpenEvent failed! " << i << endl;
				return -1;
			}
			else
			{
				cout << "OpenEvent success! " << i << endl;
			}
		}

		return 0;
	}

	return -1;
}

int CVirtualAudioAssistance::InitEngine()
{
	m_pAudioEngines[Render] = make_shared<CAudioEngine>(m_szEngineEvTaskNames[Render], SamplesPerSec, Channels, Channels, SamplesPerSec, 0);
	if (m_pAECWaveFormats)
	{
		ResetAudioEngine(m_pAudioEngines[Capture], m_szEngineEvTaskNames[Capture], SamplesPerSec, Channels, m_pAECWaveFormats->nChannels, m_pAECWaveFormats->nSamplesPerSec, CaptureAECDelay);
	}

	return 0;
}

void CVirtualAudioAssistance::ResetAudioEngine(std::shared_ptr<CAudioEngine>& pEngine, const char* pszEvTaskName, int iSampleRate, int iChannels, int iRefChannels, int iFarSampleRate, int iDelayMS)
{
	cout << "ResetAudioEngine " << pszEvTaskName << endl;
	pEngine = make_shared<CAudioEngine>(pszEvTaskName, iSampleRate, iChannels, iRefChannels, iFarSampleRate, iDelayMS);
}

int CVirtualAudioAssistance::InitResampler()
{
	// 创建重采样器

	int iErr = 0;
	if (m_pWaveFormats[Capture]->nSamplesPerSec != SamplesPerSec)
	{
		m_pResamplerStates[Capture] = speex_resampler_init(Channels, m_pWaveFormats[Capture]->nSamplesPerSec, SamplesPerSec, SPEEX_RESAMPLER_QUALITY_DEFAULT, &iErr);
		if (RESAMPLER_ERR_SUCCESS != iErr)
		{
			return -1;
		}
	}

	if (m_pWaveFormats[Render]->nSamplesPerSec != SamplesPerSec)
	{
		m_pResamplerStates[Render] = speex_resampler_init(Channels, SamplesPerSec, m_pWaveFormats[Render]->nSamplesPerSec, SPEEX_RESAMPLER_QUALITY_DEFAULT, &iErr);
		if (RESAMPLER_ERR_SUCCESS != iErr)
		{
			return -1;
		}
	}

	return 0;
}

int CVirtualAudioAssistance::InitDevice(int iType, DWORD dwStreamFlags)
{
	m_pAudioDevice[iType] = GetAudioDevice((EDeviceType)iType);
	if (!m_pAudioDevice[iType])
	{
		cout << "Get microphone device failed!" << endl;
		return -1;
	}

	if (!DeviceInitEx(m_pAudioDevice[iType], m_pAudioClient[iType], dwStreamFlags, iType))
	{
		cout << "microphone DeviceInitEx failed: " << endl;
		return -1;
	}

	if (iType == Capture)
	{
		// 获取捕获客户端
		HRESULT hr = m_pAudioClient[iType]->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(m_pAudioCaptureClient.GetAddressOf()));
		if (FAILED(hr))
		{
			cout << "microphone GetService failed: " << hr << endl;
			return -1;
		}
	}
	else if (iType == Render)
	{
		// 获取渲染客户端
		HRESULT hr = m_pAudioClient[iType]->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(m_pAudioRenderClient.GetAddressOf()));
		if (FAILED(hr))
		{
			cout << "Speaker GetService failed: " << hr << endl;
			return -1;
		}
	}

	return 0;
}

bool CVirtualAudioAssistance::SendMicrophoneData(BYTE* pData, UINT32 uiPacketLength)
{
	if (!pData || uiPacketLength == 0)
	{
		cout << "Invalid data!  uiPacketLength = " << uiPacketLength << endl;
		return false;
	}

	if (m_hIOCTL == INVALID_HANDLE_VALUE)
	{
		cout << "Invalid handle!" << endl;
		return false;
	}

	DWORD dwRet = 0;

	return DeviceIoControl(
		m_hIOCTL, // 设备句柄
		IOCTL_SEND_CAPTURE_DATA, // 控制码
		pData, // 输入缓冲区
		uiPacketLength, // 输入缓冲区大小
		NULL, // 输出缓冲区
		0, // 输出缓冲区大小
		&dwRet, // 实际输出大小
		NULL); // 重叠结构
}

void CVirtualAudioAssistance::SpeakerThread()
{
	m_pVirtualAudioClient->Start();
	BYTE* pBuffer = nullptr;
	BYTE* pSpeakerBuffer = nullptr;
	UINT32 uiPacketLength = 0;
	UINT32 uiFramesAvailable;
	DWORD  dwFlags;

	while (true)
	{
		std::unique_lock<std::mutex> lock(m_mtx[Render]);
		if (!m_bDeviceRunStatusFlags[Render].load(std::memory_order_acquire))
		{
			m_cv[Render].wait(lock, [&] { return m_bDeviceRunStatusFlags[Render].load(std::memory_order_acquire); });
		}

		HRESULT hr = m_pVirtualRenderCaptureClient->GetNextPacketSize(&uiPacketLength);
		if (FAILED(hr))
		{
			cout << "Failed to get packet size!" << endl;
		}
		if (uiPacketLength > 0)
		{
			hr = m_pVirtualRenderCaptureClient->GetBuffer(&pBuffer, &uiFramesAvailable, &dwFlags, NULL, NULL);
			if (FAILED(hr))
			{
				cout << "Failed to GetBuffer." << endl;
			}

			UINT32 uiFramesSize = uiFramesAvailable * m_pAECWaveFormats->nChannels;
			//	cout << " 2 uiFramesSize = " << uiFramesSize << endl;


			UINT32 uiPlaySize = m_pWaveFormats[Render]->nSamplesPerSec / 100;;
			hr = m_pAudioRenderClient->GetBuffer(uiPlaySize, &pSpeakerBuffer);
			if (FAILED(hr))
			{
				cout << "Failed to m_pAudioRenderClient->GetBuffer size! hr = " << hr  <<  "  size = " << uiPlaySize  << endl;
				m_pVirtualRenderCaptureClient->ReleaseBuffer(uiFramesAvailable);
				continue;
			}

			m_pAudioEngines[Render]->SyncWriteAndReadData(reinterpret_cast<const float*>(pBuffer), uiFramesSize, m_pfFarInput, uiFramesSize, reinterpret_cast<float*>(pBuffer));

			if (m_pWaveFormats[Render]->nSamplesPerSec != m_pVirtualWaveFormats->nSamplesPerSec)
			{
				UINT32 uiIn = uiFramesSize;
				uiFramesSize = m_pWaveFormats[Render]->nSamplesPerSec / 100 * m_pWaveFormats[Render]->nChannels;
				speex_resampler_process_float(m_pResamplerStates[Render], 0, reinterpret_cast<const float*>(pBuffer), &uiIn, reinterpret_cast<float*>(pSpeakerBuffer), &uiFramesSize);
			}
			else
			{
				memcpy(pSpeakerBuffer, pBuffer, uiFramesSize * sizeof(float));
			}

			m_pAudioRenderClient->ReleaseBuffer(uiPlaySize, 0);

			m_pVirtualRenderCaptureClient->ReleaseBuffer(uiFramesAvailable);
		}
		else
		{
			Sleep(1);
		}
	}
}

void CVirtualAudioAssistance::MonitorRenderStartThread()
{
	DWORD dwTimeoutMs = 0;
	while (true)
	{
		cout << "m_bDeviceRunStatusFlags[Render].load(std::memory_order_acquire) = " << m_bDeviceRunStatusFlags[Render].load(std::memory_order_acquire) << endl;
		if (WaitForSingleObject(m_mapEventName2Handle[RENDER_START_EVENT_NAME], INFINITE) == WAIT_OBJECT_0)
		{
			std::unique_lock<std::mutex> lock(m_mtx[Render]);
			cout << "MonitorRenderStartThread" << endl;
			cout << "m_bDeviceRunStatusFlags[Render].load(std::memory_order_acquire) = " << m_bDeviceRunStatusFlags[Render].load(std::memory_order_acquire) << endl;
			if (!m_bDeviceRunStatusFlags[Render].load(std::memory_order_acquire))
			{
				m_pAudioClient[Render]->Start();
				m_bDeviceRunStatusFlags[Render].store(true, std::memory_order_release);
				m_cv[Render].notify_all();
			}
		}
	}
}

void CVirtualAudioAssistance::MonitorRenderStopThread()
{
	DWORD dwTimeoutMs = 0;
	while (true)
	{
		if (WaitForSingleObject(m_mapEventName2Handle[RENDER_STOP_EVENT_NAME], INFINITE) == WAIT_OBJECT_0)
		{
			cout << "MonitorRenderStopThread" << endl;
			if (m_bDeviceRunStatusFlags[Render].load(std::memory_order_acquire))
			{
				m_pAudioClient[Render]->Stop();
				m_bDeviceRunStatusFlags[Render].store(false, std::memory_order_release);
			}
		}
	}
}

void CVirtualAudioAssistance::MonitorCaptureStartThread()
{
	DWORD dwTimeoutMs = 0;
	while (true)
	{
		if (WaitForSingleObject(m_mapEventName2Handle[CAPTURE_START_EVENT_NAME], INFINITE) == WAIT_OBJECT_0)
		{
			std::unique_lock<std::mutex> lock(m_mtx[Capture]);
			cout << "MonitorCaptureStartThread" << endl;
			if (!m_bDeviceRunStatusFlags[Capture].load(std::memory_order_acquire))
			{
				m_pReferQueue->Clear();
				ResetAudioEngine(m_pAudioEngines[Capture], m_szEngineEvTaskNames[Capture], SamplesPerSec, Channels, m_pAECWaveFormats->nChannels, m_pAECWaveFormats->nSamplesPerSec, CaptureAECDelay);
				m_pAudioClient[Capture]->Start();
				m_pAECSpeakerAudioClient->Start();
				m_bDeviceRunStatusFlags[Capture].store(true, std::memory_order_release);
				m_cv[Capture].notify_all();
			}
		}
	}
}

void CVirtualAudioAssistance::MonitorCaptureStopThread()
{
	DWORD dwTimeoutMs = 0;
	while (true)
	{
		if (WaitForSingleObject(m_mapEventName2Handle[CAPTURE_STOP_EVENT_NAME], INFINITE) == WAIT_OBJECT_0)
		{
			cout << "MonitorCaptureStopThread" << endl;
			if (m_bDeviceRunStatusFlags[Capture].load(std::memory_order_acquire))
			{
				m_pAudioClient[Capture]->Stop();
				m_bDeviceRunStatusFlags[Capture].store(false, std::memory_order_release);
				m_pAECSpeakerAudioClient->Stop();
			}
		}
	}
}

void CVirtualAudioAssistance::MicrophoneThread()
{
	LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);

	BYTE* pBuffer = nullptr;
	DWORD dwFlags;
	UINT32 uiPacketLength = 0;
	bool bAEC = false;
	float* pfTemp = new float[3840] {0};
	int32_t* piTemp = new int32_t[3840]{ 0 };
	float* pReferData = new float[m_ullReferPacketSize] {0};
	QueryPerformanceCounter(&StartingTime);
	while (true)
	{
		std::unique_lock<std::mutex> lock(m_mtx[Capture]);
		if (!m_bDeviceRunStatusFlags[Capture].load(std::memory_order_acquire))
		{
			m_cv[Capture].wait(lock, [&] { return m_bDeviceRunStatusFlags[Capture].load(std::memory_order_acquire); });
		}

		uiPacketLength = 0;

		auto hr = m_pAudioCaptureClient->GetNextPacketSize(&uiPacketLength);
		if (FAILED(hr))
		{
			cout << "microphone Error getting next packet size: " << hr << endl;
			break;
		}

		if (uiPacketLength > 0)
		{
			hr = m_pAudioCaptureClient->GetBuffer(&pBuffer, &uiPacketLength, &dwFlags, nullptr, nullptr);
			if (FAILED(hr))
			{
				cout << "microphone GetBuffer failed: " << hr << endl;
				break;
			}

			CaptureReferData();

			DWORD dwDataSize = uiPacketLength * m_pWaveFormats[Capture]->nChannels;
			if (m_pWaveFormats[Capture]->nSamplesPerSec != SamplesPerSec)
			{
				UINT32 uiIn = dwDataSize;
				UINT32 uiOut = SamplesPerSec / 100 * Channels;
				speex_resampler_process_float(m_pResamplerStates[Capture], 0, reinterpret_cast<float*>(pBuffer), &uiIn, reinterpret_cast<float*>(pfTemp), &uiOut);
				dwDataSize = uiOut;
			}
			else
			{
				memcpy(pfTemp, pBuffer, dwDataSize * sizeof(float));
			}

			if (m_pReferQueue->GetBufferSize() >= m_ullReferPacketSize * sizeof(float))
			{
				m_pReferQueue->Dequeue(pReferData, m_ullReferPacketSize * sizeof(float));
				bAEC = true;
			}
			else
			{
			}

			m_pAudioEngines[Capture]->SyncWriteAndReadData(reinterpret_cast<const float*>(pfTemp), dwDataSize, pReferData, m_ullReferPacketSize, pfTemp);
			if (bAEC)
			{
				memset(pReferData, 0, m_ullReferPacketSize);
				bAEC = false;
			}

			// 将浮点数据转换为32位整数PCM
			Float32ConvertToInt32(piTemp, pfTemp, dwDataSize);
			if (!SendMicrophoneData(reinterpret_cast<BYTE*>(piTemp), dwDataSize * sizeof(int32_t)))
			{
				cout << "SendMicrophoneData false! " << dwDataSize << endl;
			}

			hr = m_pAudioCaptureClient->ReleaseBuffer(uiPacketLength);

			if (FAILED(hr))
			{
				cout << "microphone ReleaseBuffer failed: " << hr << endl;
				//break;
			}
		}
		else
		{
			Sleep(1);
		}
	}

	delete[] pfTemp;
	delete[] piTemp;
}

void CVirtualAudioAssistance::CaptureReferData()
{
	BYTE* pBuffer = nullptr;
	UINT32 uiPacketLength = 0;
	UINT32 uiFramesAvailable;
	DWORD  dwFlags;

	try
	{
		HRESULT hr = m_pAECSpeakerCaptureClient->GetNextPacketSize(&uiPacketLength);
		if (FAILED(hr))
		{
			cout << "Failed to get packet size!" << endl;
		}
		if (uiPacketLength > 0)
		{
			hr = m_pAECSpeakerCaptureClient->GetBuffer(&pBuffer, &uiFramesAvailable, &dwFlags, NULL, NULL);
			if (FAILED(hr))
			{
				cout << "Failed to GetBuffer." << endl;
			}

			UINT32 uiFramesSize = uiFramesAvailable * m_pAECWaveFormats->nChannels;

			if (uiFramesSize > m_ullReferPacketSize)
			{
				cout << "uiFramesAvailable: " << uiFramesSize << endl;
			}

			if (!m_pReferQueue->Enqueue(pBuffer, uiFramesSize * sizeof(float)))
			{
				cout << "CaptureReferData Enqueue failed! 2" << endl;
			}

			hr = m_pAECSpeakerCaptureClient->ReleaseBuffer(uiFramesAvailable);
			if (FAILED(hr))
			{
				cout << " m_pAECSpeakerCaptureClient->ReleaseBuffe failed! 2" << endl;
			}
		}
	}
	catch (std::exception* e)
	{
		cout << "exception" << endl;
	}
}

void CVirtualAudioAssistance::InitReferCapture()
{
	cout << "InitReferCapture" << endl;
	ComPtr<IMMDeviceEnumerator> pDeviceEnumerator;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pDeviceEnumerator));

	if (FAILED(hr))
	{
		cout << "CoCreateInstance failed!" << endl;
		return;
	}

	// get the default render endpoint
	hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pAECAudioDevice);
	if (FAILED(hr))
	{
		cout << "GetDefaultAudioEndpoint failed!" << endl;
		return;
	}

	hr = CompareDevices(m_pAECAudioDevice, m_pVirtualAudioDevice, m_bReferIsVAD);
	if (FAILED(hr))
	{
		cout << "CompareDevices false!" << endl;
		return;
	}

	if (m_bReferIsVAD)
	{
		m_pAECAudioDevice = m_pAudioDevice[Render];
	}

	// activate an IAudioClient
	hr = m_pAECAudioDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(m_pAECSpeakerAudioClient.GetAddressOf()));
	if (FAILED(hr))
	{
		cout << "Activate failed: " << hr << endl;
		return;
	}
	// get the default device format
	hr = m_pAECSpeakerAudioClient->GetMixFormat(&m_pAECWaveFormats);
	if (FAILED(hr))
	{
		cout << "GetMixFormat failed!" << endl;
		return;
	}

	// 打印音频格式
	cout << "WaveFormat: " << endl;
	cout << "wFormatTag: " << m_pAECWaveFormats->wFormatTag << endl;
	cout << "nChannels: " << m_pAECWaveFormats->nChannels << endl;
	cout << "nSamplesPerSec: " << m_pAECWaveFormats->nSamplesPerSec << endl;
	cout << "nAvgBytesPerSec: " << m_pAECWaveFormats->nAvgBytesPerSec << endl;
	cout << "nBlockAlign: " << m_pAECWaveFormats->nBlockAlign << endl;
	cout << "wBitsPerSample: " << m_pAECWaveFormats->wBitsPerSample << endl;
	cout << "cbSize: " << m_pAECWaveFormats->cbSize << endl;

	DWORD dwStreamFlags = AUDCLNT_STREAMFLAGS_LOOPBACK;

	hr = m_pAECSpeakerAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, dwStreamFlags, REFTIMES_PER_SEC, 0, m_pAECWaveFormats, nullptr);
	if (FAILED(hr))
	{
		cout << "Initialize failed! hr = " << hr << endl;
		return;
	}
	else
	{
		cout << "Initialize success! iType = AEC" << endl;
	}

	hr = m_pAECSpeakerAudioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(m_pAECSpeakerCaptureClient.GetAddressOf()));
	if (FAILED(hr))
	{
		cout << "GetService failed! hr = " << hr << endl;
		return;
	}

	m_ullReferPacketSize = m_pAECWaveFormats->nSamplesPerSec / 100 * m_pAECWaveFormats->nChannels;

}

void CVirtualAudioAssistance::InitVirtualRenderCapture()
{
	m_pVirtualAudioDevice = GetAudioDevice(Render, true);
	if (!m_pVirtualAudioDevice)
	{
		cout << "Get pVirtualDevice false !" << endl;
	}

	// activate an IAudioClient
	HRESULT  hr = m_pVirtualAudioDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)m_pVirtualAudioClient.GetAddressOf());
	if (FAILED(hr))
	{
		cout << "Activate failed!" << endl;
		return;
	}

	// get the default device format
	hr = m_pVirtualAudioClient->GetMixFormat(&m_pVirtualWaveFormats);
	if (FAILED(hr))
	{
		cout << "GetMixFormat failed!" << endl;
		return;
	}

	hr = m_pVirtualAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, REFTIMES_PER_SEC, 0, m_pVirtualWaveFormats, 0);
	if (FAILED(hr))
	{
		cout << "Initialize failed!" << endl;
		return;
	}

	// activate an IAudioCaptureClient
	hr = m_pVirtualAudioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(m_pVirtualRenderCaptureClient.GetAddressOf()));
	if (FAILED(hr))
	{
		cout << "GetService failed!" << endl;
		return;
	}

	m_ullVirtualRenderPacketSize = m_pVirtualWaveFormats->nSamplesPerSec / 100 * m_pVirtualWaveFormats->nChannels;
}

void CVirtualAudioAssistance::ConvertPCM32toFloat(const int32_t* pPcmData, float* pFloatData, ULONG ulSize)
{
	if (!pPcmData || !pFloatData || ulSize == 0)
	{
		return;
	}

	const double scale = 1.0 / 2147483648.0;
	for (size_t i = 0; i < ulSize; ++i)
	{
		double temp = static_cast<double>(pPcmData[i]) * scale;
		pFloatData[i] = static_cast<float>(std::clamp(temp, -1.0, 1.0));
	}
}

void CVirtualAudioAssistance::Float32ConvertToInt32(int32_t* pPcmData, float* pFloatData, ULONG ulSize)
{
	if (!pPcmData || !pFloatData || ulSize == 0)
	{
		return;
	}
	// 将浮点数据转换为32位整数PCM
	for (DWORD i = 0; i < ulSize; ++i)
	{
		float fSample = pFloatData[i];
		int32_t iPcmSample = static_cast<int32_t>(max(min(fSample * 2147483647.0f, 2147483647.0f), -2147483648.0f));
		pPcmData[i] = iPcmSample;
	}
}

ComPtr<IMMDevice> CVirtualAudioAssistance::GetAudioDevice(const EDeviceType& eType, bool bIsVirtual/* = false*/)
{
	ComPtr<IMMDeviceEnumerator> pDeviceEnumerator;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pDeviceEnumerator));
	if (FAILED(hr))
	{
		return nullptr;
	}

	ComPtr<IMMDeviceCollection> pDeviceCollection;
	// 根据传入的设备类型选择 eRender 或 eCapture
	EDataFlow flow = (eType == Render) ? eRender : eCapture;
	hr = pDeviceEnumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &pDeviceCollection);
	if (FAILED(hr))
	{
		return nullptr;
	}

	UINT uiCount;
	pDeviceCollection->GetCount(&uiCount);

	for (UINT i = 0; i < uiCount; i++)
	{
		ComPtr<IMMDevice> pDevice;
		hr = pDeviceCollection->Item(i, &pDevice);
		if (FAILED(hr) || !pDevice)
		{
			continue;
		}

		ComPtr<IPropertyStore> pStore;
		hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);
		if (FAILED(hr))
		{
			continue;
		}

		PROPVARIANT prop;
		PropVariantInit(&prop);
		hr = pStore->GetValue(PKEY_AudioEndpoint_FormFactor, &prop);
		// 根据设备类型检查形式因子
		if (SUCCEEDED(hr) && prop.vt == VT_UI4 &&
			((eType == Render && prop.ulVal == Speakers) || (eType == Capture && prop.ulVal == Microphone)))
		{
			//打印设备名称
			PROPVARIANT pv;
			hr = pStore->GetValue(PKEY_Device_FriendlyName, &pv);
			if (SUCCEEDED(hr))
			{
				wprintf(L"Device Name: %s\n", pv.pwszVal);
				//判断名称是否包含 Virtual
				if (!bIsVirtual && wcsstr(pv.pwszVal, L"Virtual")
					|| bIsVirtual && !wcsstr(pv.pwszVal, L"Virtual"))
				{
					PropVariantClear(&pv);
					continue;
				}

				PropVariantClear(&pv);
			}

			// 打印设备ID
			LPWSTR pwszID = nullptr;
			hr = pDevice->GetId(&pwszID);
			if (SUCCEEDED(hr))
			{
				wprintf(L"Device ID: %s\n", pwszID);
			}

			//打印Hardware ID
			PROPVARIANT pvHardwareID;
			hr = pStore->GetValue(PKEY_Device_HardwareIds, &pvHardwareID);
			if (SUCCEEDED(hr))
			{
				wprintf(L"Hardware ID: %s\n", pvHardwareID.pwszVal);
				PropVariantClear(&pvHardwareID);
			}


			CoTaskMemFree(pwszID);

			PropVariantClear(&prop);
			return pDevice;
		}

		// 清理
		PropVariantClear(&prop);
	}

	return nullptr;
}

bool CVirtualAudioAssistance::DeviceInitEx(ComPtr<IMMDevice>& pDevice, ComPtr<IAudioClient2>& pAudioClient, DWORD dwStreamFlags, int iType)
{
	if (!pDevice || iType >= DeviceTypeMax)
	{
		return false;
	}

	// 激活音频客户端
	HRESULT hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(pAudioClient.GetAddressOf()));
	if (FAILED(hr))
	{
		cout << "Activate failed: " << hr << endl;
		return false;
	}

	AudioClientProperties properties = { 0 };
	properties.cbSize = sizeof(AudioClientProperties);
	properties.bIsOffload = FALSE;
	properties.eCategory = AudioCategory_Media;
	properties.Options = AUDCLNT_STREAMOPTIONS_RAW;
	hr = pAudioClient->SetClientProperties(&properties);
	if (FAILED(hr))
	{
		cout << "SetClientProperties failed: " << hr << endl;
	}

	pAudioClient->GetMixFormat(&m_pWaveFormats[iType]);
	// 打印音频格式
	cout << "WaveFormat: " << endl;
	cout << "wFormatTag: " << m_pWaveFormats[iType]->wFormatTag << endl;
	cout << "nChannels: " << m_pWaveFormats[iType]->nChannels << endl;
	cout << "nSamplesPerSec: " << m_pWaveFormats[iType]->nSamplesPerSec << endl;
	cout << "nAvgBytesPerSec: " << m_pWaveFormats[iType]->nAvgBytesPerSec << endl;
	cout << "nBlockAlign: " << m_pWaveFormats[iType]->nBlockAlign << endl;
	cout << "wBitsPerSample: " << m_pWaveFormats[iType]->wBitsPerSample << endl;
	cout << "cbSize: " << m_pWaveFormats[iType]->cbSize << endl;

	// 使用期望的格式初始化音频流
	hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, dwStreamFlags, REFTIMES_PER_SEC, 0, m_pWaveFormats[iType], nullptr);
	if (FAILED(hr))
	{
		cout << "Initialize failed: " << hr << endl;
		return false;
	}
	else
	{
		cout << "Initialize success! iType = " << iType << endl;
	}

	if (dwStreamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK)
	{
		// 创建事件句柄
		m_AudioStreamEventHandles[iType] = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_AudioStreamEventHandles[iType] == nullptr)
		{
			cout << "Failed to create event handle." << endl;
			return false;
		}

		// 设置事件句柄
		hr = pAudioClient->SetEventHandle(m_AudioStreamEventHandles[iType]);
		if (FAILED(hr))
		{
			cout << "SetEventHandle failed: " << hr << endl;
			CloseHandle(m_AudioStreamEventHandles[iType]);
			return false;
		}
	}

	return true;
}
