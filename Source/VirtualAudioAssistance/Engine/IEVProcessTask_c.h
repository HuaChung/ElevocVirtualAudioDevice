//
//  Created by elevoc on 2019/4/23.
//  Copyright © 2019年 elevoc. All rights reserved.
//

#ifndef IEVPROCESSTASK_C_H
#define IEVPROCESSTASK_C_H

#include "EVProcessEnumType.h"
#include "ev_export.h"

#ifdef __cplusplus
extern "C" {
#endif

//=================================必要方法 Start========================================
/**
 * 获取降噪处理的对象实例,支持 一次 创建 多个实例
 * @param key 与实例绑定的签名，可以通过 签名 获取 该实例，签名 重复则获取之前创建的
 * @param async_desc 表示 相应的实例 是否 开启异步 处理， true 为 异步， false 为同步
 * @return
 */
EVEXPORT void* elevoc_new_evtask(const char * key , bool async_desc);

/**
 * 通过key 来获取 指定的 实例
 * @param key 与实例 相关的 签名
 * @return evtask 实例
 */
EVEXPORT void *elevoc_get_evtask(const char* key);

/**
 * 替换 不同 的 算法 路径
 * @param evTask 针对的实例
 * @param pipelineJson 替换 算法的 json串
 */
EVEXPORT void elevoc_reset_pipeline(void* evTask,const char* pipelineJson);

/**
 *	关于apo管理
 */
EVEXPORT bool elevoc_set_process_apoarr(void *evTask, long apoAddr);

EVEXPORT long elevoc_get_process_apoarr(void *evTask);

EVEXPORT void elevoc_remove_apoarr(void *evTask, long apoAddr);


/**
 *	GNA
 */
//EVEXPORT void* elevoc_new_inference_engine_gna(void *evTask, const char *model_file_path, int mode);
//EVEXPORT void elevoc_set_inference_engine_gna(void* evTask, void* ns);
//EVEXPORT void *elevoc_get_vino_engine(void *evTask);
//EVEXPORT void elevoc_release_vino_engine(void *evTask);

/**
 * 设置GNA 的em 参数
 * @param evTask 实例
 * @param spk_em em 的 内容
 * @param len spk_em的长度
 * @return
 */
EVEXPORT bool elevoc_set_spk_em_api(void *evTask, float *spk_em,int len);


/**
 * 释放降噪处理的对象实例
 * @param evTask
 * @return
 */
EVEXPORT bool elevoc_delete_evtask(void *evTask);

 /**
  * 设置啸叫检测回调函数
  * @param evTask
  * @param detectCallBack
  * @return
  */
EVEXPORT bool elevoc_set_howling_detect_callback(void *evTask, howlingDetectCallBack detectCallBack);

/**
 * 开启降噪任务
 * @param evTask
 * @return
 */
EVEXPORT bool elevoc_start_task(void *evTask);

/**
 * 清空buffer
 * @param evTask
 * @param frameSize 清空的同时，需要强制输出多少帧静音的数据，默认10帧
 * @return
 */
EVEXPORT bool elevoc_clean_buffer(void *evTask, int frameSize = 10);


/**
 * [异步]写入原始数据
 * @param evTask
 * @param audioData 数据指针
 * @param len 数据长度（byte）
 * @return
 */
EVEXPORT bool elevoc_write_audio(void *evTask, const float *audioData, size_t len);


/**
 * [异步]写入远端数据，仅当SDK支持AEC时才可使用此接口
 * @param evTask
 * @param audioData 数据指针
 * @param len 数据长度（byte）
 * @return
 */
EVEXPORT bool elevoc_write_farend_audio(void *evTask, const float *audioData, size_t len);

 /**
  * [同步]一次写入 近端，远端，和输出 处理后的数据
  * @param evTask
  * @param near 近端数据
  * @param near_len 长度
  * @param far 远端数据
  * @param far_len 长度
  * @param output 输出数据，长度与近端数据长度一致
  * @return 读取到的字节数
  */
EVEXPORT int
elevoc_audio_process_float(void *evTask, const float *near, size_t near_len, const float *far, size_t far_len,
                           float *output);

/**
 * 针对kws场景 需要 标记 新的一次识别唤醒开始
 * @param evTask 实例
 */
EVEXPORT void elevoc_reset_kws_state(void *evTask);


/**
 * [异步]获取降噪后的数据
 * @param evTask
 * @param buffer 数据指针
 * @param len 读取的长度
 * @param isLast 是否为最后一帧（如离线处理需要在最后一帧做一些关闭文件等操作）
 * @param isPaddingZero 是否在SDK处理已数据长度不足时padding 0，如为实时语音通话场景，建议设为true
 * @return
 */
EVEXPORT size_t elevoc_read_buffer(void *evTask, float *buffer, size_t len, bool *isLast, bool isPaddingZero = false);


/**
 * 停止降噪
 * @param evTask
 * @param stopImmediately 是否立即停止（将丢弃队列中还未处理的数据，实时语音通话场景建议为true）
 * @return
 */
EVEXPORT bool elevoc_stop_task(void *evTask, bool stopImmediately = true);


/**
 * 获取SDK的版本号
 * @param evTask
 * @return
 */
EVEXPORT const char *elevoc_lib_ver(void *evTask);


 /**
  * 设置 近端 delay 的 采样数
  * @param evTask
  * @param delay_samples delay采样点数
  */
EVEXPORT void elevoc_set_delay_ms(void *evTask, int delay_ms);

/**
 * 设置是否启用log日志，开启日志有助于定位问题
 * @param evTask
 * @param enableLog true / false
 * @return
 */
EVEXPORT bool elevoc_set_enable_log(void *evTask, bool enableLog);

/**
 * 设置dump语音数据的目录
 * @param evTask
 * @param dir
 */
EVEXPORT void elevoc_set_dump_audio_dir(void *evTask, const char *dir);

/**
 * 设置是否开启语音数据dump, 默认是关闭
 * @param evTask
 * @param enable
 */
EVEXPORT void elevoc_set_enable_dump_audio(void *evTask, bool enable);

/**
* 设置近端采样率
@param sampleRate 8000 or 16000 or 32000 or 48000
*/
EVEXPORT void elevoc_set_samplerate(void *evTask,int sampleRate);

/**
* 设置远端采样率
* @param farSampleRate 近端信号 采样率8000 or 16000 or 32000 or 44100 or 48000
*/
EVEXPORT void elevoc_set_far_samplerate(void *evTask,int farSampleRate);

/**
 * 设置 近端 通道数
 * @param evTask
 * @param channels 通道数
 */
EVEXPORT void elevoc_set_channels(void *evTask,int channels);

/**
 *  设置远端 采样率
 * @param evTask
 * @param channels
 */
EVEXPORT void elevoc_set_far_channels(void *evTask,int channels);

/**
 * 给 算法 设置 自己 想要 的参数
 * @param evtask
 * @param parameter json格式
 */
EVEXPORT void elevoc_set_module_parameter(void*evtask,const char* parameter);

/**
 * 给 sdk 设置 参数
 * @param evtask
 * @param parameter json格式
 */
EVEXPORT void elevoc_set_sdk_parameter(void*evtask,const char* parameter);

#ifdef __cplusplus
}
#endif

#endif //NNSS_DEMO_IEVPROCESSTASK_C_H
