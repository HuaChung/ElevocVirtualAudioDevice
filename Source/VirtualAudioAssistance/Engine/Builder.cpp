//
// Created by Elevoc on 2021/12/14 0014.
//

#include "Builder.h"
#include <sstream>
#include <fstream>


Builder *Builder::addModule(BaseModule *module) {
    m_modules.push_back(module);
    return this;
}


Builder *Builder::setCommonParam(CommonParam *param) {
    m_param = param;
    return this;
}

Builder *Builder::setIoDesc(IoDesc *desc) {
    m_desc = desc;
    return this;
}

std::string Builder::build() {
    if (!m_param || !m_desc || m_modules.empty()) {
        printf("error!!!comm_param=%p,io_desc:%p,modules is null", m_param, m_desc);
        return "";
    }
    // 构造 Pipeline
    std::stringstream ss;
    ss << "{";
    // common_param
    ss << "\"common_param\":";
    ss << "{";
    ss << "\"samplerate\" : " << m_param->samplerate << ",";
    ss << "\"mic_num\" : " << m_param->mic_num << ",";
    ss << "\"ref_num\" : " << m_param->ref_num << ",";
    ss << "\"ori_frame_ms\" : " << m_param->ori_frame_ms << ",";
    ss << "\"fb_item_scale_factor\" : " << m_param->fb_scale_item_factor << ",";
    ss << "\"fb_scale_factor\" : " << m_param->fb_scale_factor;
    ss << "},";
    // io_desc
    ss << "\"io_desc\" :";
    ss << "{";
    ss << R"("mic_input_file" : ")" << m_desc->mic_input_file << "\",";
    ss << R"("ref_input_file" : ")" << m_desc->ref_input_file << "\",";
    ss << R"("output_file" : ")" << m_desc->output_file << "\",";
    ss << R"("data_type" : ")" << io_data_type_str[m_desc->data_type] << "\"";
    ss << "},";
    if (!m_mergeParam.empty()) {
        // merge_params
        ss << "\"merge_params\" :";
        ss << m_mergeParam.data();
        ss << ",";
    }
    // modules_params
    ss << "\"modules_params\" : ";
    ss << "[";
    BaseModule *previous = nullptr;
    BaseModule *last = m_modules.back();
    for (auto *module : m_modules) {
        ss << "{";
        ss << "\"depend\" : " << "[";
        if (!module->getDepends().empty()) {
            auto depends = module->getDepends();
            auto back = depends.back();
            for (auto &depend:depends) {
                ss << "\"" << depend << "\"";
                if (depend != back) {
                    ss << ",";
                }
            }
        } else if (previous) {
            auto outputs = previous->getOutputs();
            auto back = outputs.back();
            for (int i = 0; i < module->getDependCount(); ++i) {
                ss << "\"" << outputs[i] << "\"";
                if (i !=  module->getDependCount()-1) {
                    ss << ",";
                }
            }
        } else {
            ss << "\"\"";
        }
        ss << "],";
        ss << "\"output\" : " << "[";
        auto outputs = module->getOutputs();
        if (!outputs.empty()) {
            auto back = outputs.back();
            for (auto &output:outputs) {
                ss << "\"" << output << "\"";
                if (output != back) {
                    ss << ",";
                }
            }
        } else {
            ss << "\"\"";
        }
        ss << "],";
        ss << R"("module_name" : ")" << module->getName() << "\",";
        ss << "\"module_frame_ms\" : " << module->getFrameMs() << ",";
        ss << "\"needLoad\" : " << std::boolalpha << module->isNeedLoad() << ",";
        ss << "\"needCallback\" : " << module->isNeedCallback() << ",";
        if(module->getParam().empty()){
            ss<< R"("param" : {})";
        }else{
            ss << R"("param" : )" << module->getParam();
        }
        ss << "}";
        if (module != last) {
            ss << ",";
        }
        previous = module;
    }

    ss << "]";
    ss << "}";
    return ss.str();
}

Builder *Builder::setMergeParam(const std::string &path) {
    std::ifstream file(path.data(), std::ifstream::in);
    if (file) {
        std::stringstream ss;
        ss<<file.rdbuf();
        m_mergeParam = ss.str();
        file.close();
    }
    return this;
}
