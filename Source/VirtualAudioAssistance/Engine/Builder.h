//
// Created by Elevoc on 2021/12/14 0014.
//

#ifndef TEST1_BUILDER_H
#define TEST1_BUILDER_H


#include "BaseModule.h"

typedef struct common_param {
    int samplerate;
    int mic_num;
    int ref_num;
    int out_num;
    int ori_frame_ms;
    int fb_scale_factor = 1;
    int fb_scale_item_factor = 1;

    common_param(int samplerate_,
                 int mic_num_,
                 int ref_num_,
                 int out_num_,
                 int ori_frame_ms_) {
        samplerate = samplerate_;
        mic_num = mic_num_;
        ref_num = ref_num_;
        out_num = out_num_;
        ori_frame_ms = ori_frame_ms_;
    }
} CommonParam;

static const char *io_data_type_str[] = {"i16", "f32"};

typedef enum {
    i16,
    f32
} io_data_type;

typedef struct io_desc {
    io_data_type data_type;
    std::string mic_input_file;
    std::string ref_input_file;
    std::string output_file;

    io_desc(io_data_type data_type_) {
        data_type = data_type_;
    }
} IoDesc;

class Builder {

public:
    Builder *addModule(BaseModule *module);

    Builder *setIoDesc(IoDesc *desc);

    Builder *setCommonParam(CommonParam *param);

    Builder *setMergeParam(const std::string& path);

    std::string build();

private:
    std::vector<BaseModule *> m_modules;
    IoDesc *m_desc;
    CommonParam *m_param;
    std::string m_mergeParam;

};


#endif //TEST1_BUILDER_H
