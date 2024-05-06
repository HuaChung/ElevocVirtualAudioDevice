#include "CAudioEngine.h"
#include "BaseModule.h"
#include "IEVProcessTask_c.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

using namespace std;
using namespace nlohmann;

struct ModelParams
{
	json model_lists;
	UINT32 current_model_type;
	bool enable_low_frequency;

	json toJson();
};

json ModelParams::toJson()
{
	return json{
	{"model_lists", model_lists},
	{"current_model_type", current_model_type},
	{"enable_low_frequency",enable_low_frequency}
	};
}

struct ModelConfig
{
	std::string path;
	UINT32 type;
	FLOAT mask;
	LONG64 spk_em;
	UINT32 mic_dis;
	UINT32 angle;
	json toJson();
};

json ModelConfig::toJson()
{
	return json{
		{"model_path", path},
		{"model_type", type},
		{"model_mask", mask},
		{"spk_em", spk_em},
		{"mic_distance", mic_dis},
		{"angle", angle}
	};
}



CAudioEngine::CAudioEngine(const char* pszEvTaskName, int iSampleRate, int iChannels, int iRefChannels, int iFarSampleRate, int iDelayMS)
	: m_iSampleRate(iSampleRate),
	m_iChannels(iChannels),
	m_iRefChannels(iRefChannels),
	m_iFarSampleRate(iFarSampleRate),
	m_iDelayMS(iDelayMS)
{
	m_pEvtask = elevoc_new_evtask(pszEvTaskName, false);
	InitEngine();
}

CAudioEngine::~CAudioEngine()
{
	if (m_pEvtask)
	{
		elevoc_stop_task(m_pEvtask);
		elevoc_delete_evtask(m_pEvtask);
	}
}

void* CAudioEngine::GetEvTask()
{
	return m_pEvtask;
}

void CAudioEngine::AsyncWriteData(const float* pData, int iLen)
{
	if (!elevoc_write_audio(m_pEvtask, pData, iLen))
	{
		cout << "elevoc_write_audio false" << endl;
	}
}

void CAudioEngine::AsyncWriteFarendData(const float* pData, int iLen)
{
	if (!elevoc_write_farend_audio(m_pEvtask, pData, iLen))
	{
		cout << "elevoc_write_farend_audio false" << endl;
	}
}

void CAudioEngine::SyncWriteAndReadData(const float* pNear, size_t ulNearSize, const float* pFar, size_t ulFarSize, float* pOutput)
{
	elevoc_audio_process_float(m_pEvtask, pNear, ulNearSize, pFar, ulFarSize, pOutput);
}

void CAudioEngine::SyncWriteAndReadData(void* pEvTask, const float* pNear, size_t ulNearSize, const float* pFar, size_t ulFarSize, float* pOutput)
{
	elevoc_audio_process_float(pEvTask, pNear, ulNearSize, pFar, ulFarSize, pOutput);
}

void* CAudioEngine::NewEvTask(const char* key, bool bIsAsync)
{
	return elevoc_new_evtask(key, bIsAsync);
}

void CAudioEngine::AsyncReadData(float* pData, int iLen)
{
	bool bIsLast = false;
	elevoc_read_buffer(m_pEvtask, pData, iLen, &bIsLast, false);
}

bool CAudioEngine::InitEngine()
{
	m_pInput_Module = make_shared<InputModule>();
	m_pOutput_Module = make_shared<OutputModule>();
	m_pCns_Module = make_shared<VoipCNSModule>();
	m_pAec_Module = make_shared<VoipAECModule>();

	shared_ptr<CommonParam> pAudioParams(new CommonParam(m_iSampleRate, m_iChannels, m_iRefChannels, m_iChannels, 10));

	IoDesc ioDsc(io_data_type::f32);
	Builder cBuilder;
	cBuilder.setIoDesc(&ioDsc)
		->setCommonParam(pAudioParams.get())
		->addModule(m_pInput_Module.get())
		->addModule(m_pAec_Module.get())
		->addModule(m_pCns_Module.get())
		->addModule(m_pOutput_Module.get());

	ModelConfig model_config{ ".\\ELE_SMIC_M_C.raw", 102, 1.0f, 0 ,50, 90 };
	std::vector<json> vecModel_List_Jsons;
	vecModel_List_Jsons.emplace_back(model_config.toJson());
	ModelParams elevoc_model{ vecModel_List_Jsons, 102, true };

	json ns_moudle;
	ns_moudle["elevoc_cns"] = elevoc_model.toJson();
	m_pCns_Module->setParam(ns_moudle.dump());
	cout << ns_moudle.dump() << endl;

	cout << cBuilder.build() << endl;
	elevoc_reset_pipeline(m_pEvtask, cBuilder.build().c_str());

	elevoc_set_channels(m_pEvtask, 2);
	elevoc_set_far_channels(m_pEvtask, 2);

	elevoc_set_samplerate(m_pEvtask, m_iSampleRate);
	elevoc_set_far_samplerate(m_pEvtask, m_iFarSampleRate);

	elevoc_set_delay_ms(m_pEvtask, m_iDelayMS);

	elevoc_set_enable_dump_audio(m_pEvtask, false);
	elevoc_set_dump_audio_dir(m_pEvtask, ".");

	elevoc_start_task(m_pEvtask);

	return true;
}