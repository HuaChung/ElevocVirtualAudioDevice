//
//  EVProcessEnumType.h
//  SingleChannelEngine
//
//  Created by elevoc on 2017/12/25.
//  Copyright © 2017年 elevoc. All rights reserved.
//

#ifndef EVProcessEnumType_h
#define EVProcessEnumType_h
#include <stdio.h>

typedef enum EVProcessState {
	EVProcessInvalidState = 0,  
	EVProcessInitedState,  
	EVProcessLoadedState,   
	EVProcessStartState,   
	EVProcessPreStopState,  
	EVProcessStoppedState, 
	EVProcessReleaseState
} EVProcessState;

typedef enum {
	PROCESS_ERROR_LICENSE_VERIFY = 0,
	PROCESS_ERROR_APPID_VERIFY = 1,
	PROCESS_ERROR_STATE_ERROR = 2,
	PROCESS_ERROR_CACERT_PATH = 3,
} ProcessErrorType;

typedef enum {
	DEFAULT = 0,
	VOIP,
}MODE;

typedef enum {
	MODEL_TYPE_CPU = 0, // 大模型
	MODEL_TYPE_GNA,
	MODEL_TYPE_SINGLE, // 单麦算法,小模型
	MODEL_TYPE_VOICE_SEPARATION,
	MODEL_TYPE_TRADITION_BF_NS,		//双麦模式 bf+ns
	MODEL_TYPE_TRADITION_SIGNAL_NS, //单麦模式传统降噪
	MODEL_TYPE_DEREVERBERATION_NS,
	MODEL_TYPE_DEEP_ECHO_CANCELLATION
}MODEL_TYPE;

typedef enum {
	SINGLE_MIC_UP_NS_MODE = 1,
	DUAL_MIC_UP_NS_MODE,
	SINGLE_MIC_UP_NODELAY_NS_MODE,
	DOWN_NS_MODE,
	SPEAKER_EXTRACTION_MODE,
	DUAL_MIC_CNN_LSTM_UP_NS_MODE,
	FOUR_MIC_CNN_LSTM_UP_NS_MODE,
	FOUR_MIC_SPEAKER_EXTRACTION_MODE,
	SINGLE_MIC_UP_DEREVERBERATION_MODE,
	SINGLE_MIC_CNN_LSTM_UP_NS_MODE,
	DEEP_ECHO_CANCELLATION_MODE,             //GNA
	SINGLE_MIC_TCNN_NS_MODE, // 2 channels
	DUAL_MIC_TCNN_NS_MODE,   //
	SINGLE_MIC_1C_NS_MODE,                         // 1 channel
	SPEAKER_EXTRACTION_V2_MODE,
	SPATIAL_AUDIO_NN_MODE,   //
	BANDWIDTH_EXTENSION_NS_MODE,
	FOUR_MIC_3D_180DEG_TCNN_NS_MODE,
	SPATIAL_AUDIO_2MIC_NN_MODE,
	FOUR_MIC_3D_60DEG_TCNN_NS_MODE,
	NUM_OF_MODE
} INFERENCE_ENGINE_MODE;

typedef enum 
{
	ELEVOC_BASE = 100,
	ELEVOC_SM_D0_MDL = 101,
	ELEVOC_SM_D1_MDL = 102,
	ELEVOC_DM_D1_MDL = 103,
	ELEVOC_SM1C_TCNN_NS,
	ELEVOC_SM2C_TCNN_NS,
	ELEVOC_DM_TCNN_NS,
	ELEVOC_SM_SPEAKER_EXTRACTION,
	ELEVOC_DM_SPEAKER_EXTRACTION,
	ELEVOC_3D_SPEAKER_EXTRACTION,
	ELEVOC_DM_SPATIAL_AUDIO,
	ELEVOC_BANDWIDTH_EXTENSION,
	ELEVOC_MAX
}ELEVOC_MDL_TYPE;

typedef enum
{
	SINGLE_MIC_NS_MODE = 201,
	DUAL_MIC_NS_MODE = 202,
	SPEAKER_EXTRACTION_V1_MODE = 203,
	NOISE_SUPPRESSION_SM1C_TCNN_CPU_MODE = 204,
	NOISE_SUPPRESSION_SM2C_TCNN_CPU_MODE = 205,
	NOISE_SUPPRESSION_DM_TCNN_CPU_MODE = 206,
	NOISE_SUPPRESSION_SM1C_TCNN_HTP_MODE = 207, //CDSP FP16
	NOISE_SUPPRESSION_SM2C_TCNN_HTP_MODE = 208,
	NOISE_SUPPRESSION_DM_TCNN_HTP_MODE = 209
} QNN_ENGINE_MODE;

typedef enum {
	NO_DEVICE_MODE = 0,
	HAND_SET = 1,
	HAND_FREE = 2,
}DEVICE_VOICE_MODE;

typedef enum {
	DNN_NOISE_DECAY_BAND_0_600HZ_DB = 0,    	// (-60.0~0.0db)
	DNN_NOISE_DECAY_BAND_600_2500HZ_DB,     	// (-60.0~0.0db)
	DNN_NOISE_DECAY_BAND_2500_5000HZ_DB,    	// (-60.0~0.0db)
	DNN_NOISE_DECAY_BAND_GREATE_THAN_5000HZ_DB, // (-60.0~0.0db)
	DNN_NOISE_SENSITIVITY_LB,               // (0.0~2.0)
	DNN_NOISE_SENSITIVITY_HB,               // (0.0~2.0)
	DNN_POST_NOISE_SENSITIVITY_LB,          // (0.0~2.0)
	DNN_POST_NOISE_SENSITIVITY_HB           // (0.0~2.0)
}DNNParamType;

typedef enum {
	EVFuncInit,
	EVFuncReset
}EVFuncAction;

typedef void(*processFailCallBack)(ProcessErrorType errorType, const char* errorDesc);
typedef void(*howlingDetectCallBack)(bool isHowling);
typedef void(*oauthSuccessCallBack)(void);

// vad state
typedef void(*vadStartCallBack)();
typedef void(*vadStopCallBack)();
typedef void(*vadCancelCallBack)();
typedef void(*vadNoSpeechCallBack)();

#endif /* EVProcessEnumType_h */
